/*
 * opencog/util/concurrent_queue.h
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
 * 1) The queue here is unbounded in size
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

#ifndef _OC_CONCURRENT_QUEUE_H
#define _OC_CONCURRENT_QUEUE_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <queue>

/** \addtogroup grp_cogutil
 *  @{
 */

//! Represents a thread-safe first in-first out list.
///
/// Implements a thread-safe queue: any thread can push stuff onto the
/// queue, and any other thread can remove stuff from it.  If the queue
/// is empty, the thread attempting to remove stuff will block.  If the
/// queue is empty, and something is added to the queue, and there is
/// some thread blocked on the queue, then that thread will be woken up.
///
/// The function provided here is almost identical to that provided by
/// the pool class (also in this directory), but with a fancier API that
/// allows cancellation, and other minor utilities. This API is also
/// most easily understood as a producer-consumer API, with producer
/// threads adding stuff to the queue, and consumer threads removing
/// them.  By contrast, the pool API is a borrow-and-return API, which
/// is really more-or-less the same thing, but just uses a different
/// mindset.  This API also matches the proposed C++ standard for this
/// basic idea.

template<typename Element>
class concurrent_queue
{
private:
    std::queue<Element> the_queue;
    mutable std::mutex the_mutex;
    std::condition_variable the_cond;
    std::condition_variable _watermark_cond;
    bool is_canceled;
    size_t _high_watermark;
    size_t _low_watermark;
    std::atomic<size_t> _blocked_pushers;

    concurrent_queue(const concurrent_queue&) = delete;  // disable copying
    concurrent_queue& operator=(const concurrent_queue&) = delete; // no assign

public:
    concurrent_queue(void)
        : the_queue(), the_mutex(), the_cond(), _watermark_cond(),
          is_canceled(false),
          _high_watermark(DEFAULT_HIGH_WATER_MARK),
          _low_watermark(DEFAULT_LOW_WATER_MARK),
          _blocked_pushers(0)
    {}
    ~concurrent_queue()
    { if (not is_canceled) cancel(); }

    struct Canceled : public std::exception
    {
        const char * what() { return "Cancellation of wait on concurrent_queue"; }
    };

    // These limits seem ... reasonable ...
    static constexpr size_t DEFAULT_HIGH_WATER_MARK = INT32_MAX;
    static constexpr size_t DEFAULT_LOW_WATER_MARK = INT32_MAX - 65536;

private:
    /// Push the Element onto the queue.
    template<typename PushFunc>
    void push_impl(PushFunc&& do_push)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();

        // Block if queue is at or above high watermark,
        // wait until it drops below high watermark (with
        // hysteresis notification at low watermark).
        bool was_blocked = false;
        if (the_queue.size() >= _high_watermark)
        {
            was_blocked = true;
            _blocked_pushers++;
            while (the_queue.size() >= _high_watermark and not is_canceled)
            {
                _watermark_cond.wait(lock);
            }
            _blocked_pushers--;
            if (is_canceled) throw Canceled();
        }

        do_push();

        // If this pusher was blocked and there are still
        // blocked pushers, wake one more so they can proceed.
        bool should_cascade = (was_blocked and _blocked_pushers > 0);

        lock.unlock();
        the_cond.notify_one();

        if (should_cascade)
            _watermark_cond.notify_one();
    }

public:
    void push(const Element& item)
    {
        push_impl([&]() { the_queue.push(item); });
    }
    void push(Element&& item)
    {
        push_impl([&]() { the_queue.push(std::move(item)); });
    }

    /// Return true if the queue is empty at this instant in time.
    /// Since other threads may have pushed or popped immediately
    /// after this call, the emptiness of the queue may have
    /// changed by the time the caller looks at it.
    bool is_empty() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();
        return the_queue.empty();
    }

    /// Return true if the queue is at/above high watermark or has
    /// blocked pushers.
    bool is_full() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_queue.size() >= _high_watermark or _blocked_pushers > 0;
    }

    /// Return the size of the queue at this instant in time.
    /// Since other threads may have pushed or popped immediately
    /// after this call, the size may have become incorrect by
    /// the time the caller looks at it.
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_queue.size();
    }

    std::queue<Element> peek() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        std::queue<Element> copy(the_queue);
        return copy;
    }

#define COMMON_POP_NOTIFY {                               \
        value = the_queue.front();                        \
        the_queue.pop();                                  \
        /* Wake up waiting pushers when dropping below */ \
        /* low watermark. (hysteresis)                 */ \
        bool should_notify = (_blocked_pushers > 0) and   \
                             (the_queue.size() < _low_watermark); \
        lock.unlock();                                    \
        if (should_notify)                                \
            _watermark_cond.notify_all(); }

    /// Try to get an element off the front of the queue. Return true
    /// if success, else return false. This will work even on closed
    /// queues, and so can be used to drain the queue. Another
    /// alternative for closed queues is the wait_and_take_all()
    /// method, which blocks on open queues, and empties closed ones.
    bool try_get(Element& value)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        if (the_queue.empty())
        {
            return false;
        }
        COMMON_POP_NOTIFY
        return true;
    }
    bool try_pop(Element& value) { return try_get(value); }

#define COMMON_COND_WAIT(DO_THING)                           \
        std::unique_lock<std::mutex> lock(the_mutex);        \
        /* Use two nested loops here.  It can happen that */ \
        /* the cond wakes up, and yet the queue is empty. */ \
        do {                                                 \
            while (the_queue.empty() and not is_canceled)    \
                the_cond.wait(lock);                         \
            if (is_canceled) DO_THING;                       \
        } while (the_queue.empty());

    /// Pop an item off the queue. Block if the queue is empty.
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

    std::queue<Element> wait_and_take_all()
    {
        COMMON_COND_WAIT({ break; })

        std::queue<Element> retval;
        std::swap(retval, the_queue);
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

        while (the_queue.empty() and not is_canceled)
        {
            the_cond.wait(lock);
        }
        if (is_canceled) throw Canceled();
    }

    /// Set the high and low watermarks for the queue.
    /// When the queue size reaches or exceeds the high watermark,
    /// push() operations will block until the size drops below
    /// the high watermark. The low watermark provides hysteresis:
    /// notifications to blocked threads occur when size drops below
    /// the low watermark, allowing multiple blocked threads to proceed
    /// without excessive wakeup/sleep cycling.
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

#endif // __CONCURRENT_QUEUE__
