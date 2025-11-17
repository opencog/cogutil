/*
 * opencog/util/concurrent_stack.h
 *
 * Based off of http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
 * Original version by Anthony Williams
 * Modifications by Michael Anderson
 * Modified by Linas Vepstas
 * Updated API to more closely resemble the proposed
 * ISO/IEC JTC1 SC22 WG21 N3533 C++ Concurrent Queues
 * ISO/IEC JTC1 SC22 WG21 P0260R3 C++ Concurrent Queues
 *
 * This differs from P0260R3 in that:
 * 1) The stack here is unbounded in size
 * 2) It doesn't have iterators
 * 3) There is no try_put()
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OC_CONCURRENT_STACK_H
#define _OC_CONCURRENT_STACK_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <stack>

/** \addtogroup grp_cogutil
 *  @{
 */

//! Represents a thread-safe first in-first out list.
///
/// Implements a thread-safe stack: any thread can push stuff onto the
/// stack, and any other thread can remove stuff from it.  If the stack
/// is empty, the thread attempting to remove stuff will block.  If the
/// stack is empty, and something is added to the stack, and there is
/// some thread blocked on the stack, then that thread will be woken up.
///
/// The function provided here is almost identical to that provided by
/// the pool class (also in this directory), but with a fancier API that
/// allows cancellation, and other minor utilities. This API is also
/// most easily understood as a producer-consumer API, with producer
/// threads adding stuff to the stack, and consumer threads removing
/// them.  By contrast, the pool API is a borrow-and-return API, which
/// is really more-or-less the same thing, but just uses a different
/// mindset.  This API also matches the proposed C++ standard for this
/// basic idea.

template<typename Element>
class concurrent_stack
{
private:
    std::stack<Element> the_stack;
    mutable std::mutex the_mutex;
    std::condition_variable the_cond;
    std::condition_variable _watermark_cond;
    bool is_canceled;
    size_t _high_watermark;
    size_t _low_watermark;
    std::atomic<size_t> _blocked_pushers;

    concurrent_stack(const concurrent_stack&) = delete;  // disable copying
    concurrent_stack& operator=(const concurrent_stack&) = delete; // no assign

public:
    concurrent_stack(void)
        : the_stack(), the_mutex(), the_cond(), _watermark_cond(),
          is_canceled(false),
          _high_watermark(DEFAULT_HIGH_WATER_MARK),
          _low_watermark(DEFAULT_LOW_WATER_MARK),
          _blocked_pushers(0)
    {}
    ~concurrent_stack()
    { if (not is_canceled) cancel(); }

    struct Canceled : public std::exception
    {
        const char * what() { return "Cancellation of wait on concurrent_stack"; }
    };

    // These limits seem ... reasonable ...
    static constexpr size_t DEFAULT_HIGH_WATER_MARK = INT32_MAX;
    static constexpr size_t DEFAULT_LOW_WATER_MARK = INT32_MAX - 65536;

private:
    /// Push the Element onto the stack.
    template<typename PushFunc>
    void push_impl(PushFunc&& do_push)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();

        // Block if stack is at or above high watermark
        bool was_blocked = false;
        if (the_stack.size() >= _high_watermark)
        {
            was_blocked = true;
            _blocked_pushers++;
            while (the_stack.size() >= _high_watermark and not is_canceled)
            {
                _watermark_cond.wait(lock);
            }
            _blocked_pushers--;
            if (is_canceled) throw Canceled();
        }

        do_push();

        // If this pusher was blocked and there are still blocked pushers
        // waiting, notify them all so they can check if there's room.
        bool should_cascade = (was_blocked and _blocked_pushers > 0);

        lock.unlock();
        the_cond.notify_one();

        if (should_cascade)
            _watermark_cond.notify_all();
    }

public:
    void push(const Element& item)
    {
        push_impl([&]() { the_stack.push(item); });
    }
    void push(Element&& item)
    {
        push_impl([&]() { the_stack.push(std::move(item)); });
    }

    /// Return true if the stack is empty at this instant in time.
    /// Since other threads may have pushed or popped immediately
    /// after this call, the emptiness of the stack may have
    /// changed by the time the caller looks at it.
    bool is_empty() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();
        return the_stack.empty();
    }

    /// Return true if the stack is at/above high watermark or has blocked pushers.
    bool is_full() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_stack.size() >= _high_watermark or _blocked_pushers > 0;
    }

    /// Return the size of the stack at this instant in time.
    /// Since other threads may have pushed or popped immediately
    /// after this call, the size may have become incorrect by
    /// the time the caller looks at it.
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_stack.size();
    }

    std::stack<Element> peek() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        std::stack<Element> copy(the_stack);
        return copy;
    }

#define COMMON_POP_NOTIFY {                               \
        value = the_stack.top();                          \
        the_stack.pop();                                  \
        /* Wake up waiting pushers when dropping below */ \
        /* low watermark. (hysteresis)                 */ \
        bool should_notify = (_blocked_pushers > 0) and   \
                             (the_stack.size() < _low_watermark); \
        lock.unlock();                                    \
        if (should_notify)                                \
            _watermark_cond.notify_all(); }

    /// Try to pop an element off the top of the stack. Return true
    /// if success, else return false. This will work even on closed
    /// stacks, and so can be used to clear the stack. Another
    /// alternative for closed stacks is the wait_and_take_all()
    /// method, which blocks on open stacks, and empties closed ones.
    bool try_pop(Element& value)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        if (the_stack.empty())
        {
            return false;
        }

        COMMON_POP_NOTIFY
        return true;
    }

#define COMMON_COND_WAIT(DO_THING)                           \
        std::unique_lock<std::mutex> lock(the_mutex);        \
        /* Use two nested loops here.  It can happen that */ \
        /* the cond wakes up, and yet the queue is empty. */ \
        do {                                                 \
            while (the_stack.empty() and not is_canceled)    \
                the_cond.wait(lock);                         \
            if (is_canceled) DO_THING;                       \
        } while (the_stack.empty());

    /// Pop an item off the stack. Block if the stack is empty.
    void pop(Element& value)
    {
        COMMON_COND_WAIT({ throw Canceled(); })
        COMMON_POP_NOTIFY
    }
    void wait_pop(Element& value) { pop(value); }

#undef COMMON_POP_NOTIFY

    Element value_pop()
    {
        Element value;
        pop(value);
        return value;
    }

    std::stack<Element> wait_and_take_all()
    {
        COMMON_COND_WAIT({ break; })

        std::stack<Element> retval;
        the_stack.swap(retval);
        return retval;
    }
#undef COMMON_COND_WAIT

    /// A weak barrier.  This will block as long as the queue is empty,
    /// returning only when the queue isn't. It's "weak", because while
    /// it waits, other threads may push and then pop something from
    /// the queue, while this thread slept the entire time. However,
    /// if this call does return, then the queue is almost surely not
    /// empty.  "Almost surely" means that none of the other threads
    /// that are currently waiting to pop from the queue will be woken.
    void barrier()
    {
        std::unique_lock<std::mutex> lock(the_mutex);

        while (the_stack.empty() and not is_canceled)
        {
            the_cond.wait(lock);
        }
        if (is_canceled) throw Canceled();
    }

    /// Set the high and low watermarks for the stack.
    /// When the stack size reaches or exceeds the high watermark,
    /// push() operations will block until the size drops below
    /// the low watermark.
    void set_watermarks(size_t high, size_t low)
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        _high_watermark = high;
        _low_watermark = low;
    }

    void cancel_reset()
    {
       // This doesn't lose data, but it instead allows new calls
       // to not throw Canceled exceptions
       std::lock_guard<std::mutex> lock(the_mutex);
       is_canceled = false;
    }
    void open() { cancel_reset(); }

    void cancel()
    {
       std::unique_lock<std::mutex> lock(the_mutex);
       if (is_canceled) throw Canceled();
       is_canceled = true;
       lock.unlock();
       the_cond.notify_all();
       _watermark_cond.notify_all();
    }
    void close() { cancel(); }

    bool is_closed() const noexcept { return is_canceled; }

    static bool is_lock_free() noexcept { return false; }
};
/** @}*/

#endif // __CONCURRENT_STACK__
