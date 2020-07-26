/*
 * opencog/util/concurrent_set.h
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
 * 1) The set here is unbounded in size
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

#ifndef _OC_CONCURRENT_SET_H
#define _OC_CONCURRENT_SET_H

#include <condition_variable>
#include <set>
#include <exception>
#include <mutex>

/** \addtogroup grp_cogutil
 *  @{
 */

//! Represents a thread-safe std::set. Because a set can only ever hold
/// a single copy of an item, this provides a basic de-duplication
/// service.
///
/// This implements a thread-safe set: any thread can insert stuff into
/// the set, and any other thread can remove stuff from it.  If the set
/// is empty, the thread attempting to remove stuff will block.  If the
/// set is empty, and something is added to the set, and there is
/// some thread blocked on the set, then that thread will be woken up.
///
/// The function provided here is almost identical to that provided by
/// the concurrent_stack/concurrent_queue classes (in this directory)
/// except that the class behaves like a set: an item can be inserted
/// into the set many times, and, if its not removed, then it will only
/// ever appear in it once.  In essence, it avoids redundancy, as
/// compared to items placed in a queue/stack.
///
/// In other respects, it behaves like the concurrent queue/stack
/// classes: when an item is gotten, it is removed (erased) from the set.
/// It employes the same cancellation and condition-wait mechanisms
/// as those classes, thus ensuring proper concurrency.
///
/// The Element class must have an std::less associated with it, as that
/// is used to determine if an element is already in the set.  When
/// getting elements from the set, they will be gotten from the "front",
/// i.e. based on what std::less returned.  This means that there are no
/// round-robin or "fair" fetching guarantees of any sort; in fact, the
/// gets are guaranteed to NOT be fair.  This may result in unexpected
/// behavior.

template<typename Element>
class concurrent_set
{
private:
    std::set<Element> the_set;
    mutable std::mutex the_mutex;
    std::condition_variable the_cond;
    bool is_canceled;

    concurrent_set(const concurrent_set&) = delete;  // disable copying
    concurrent_set& operator=(const concurrent_set&) = delete; // no assign

public:
    concurrent_set(void)
        : the_set(), the_mutex(), the_cond(), is_canceled(false)
    {}
    ~concurrent_set()
    { if (not is_canceled) cancel(); }

    struct Canceled : public std::exception
    {
        const char * what() { return "Cancellation of wait on concurrent_set"; }
    };

    /// Insert the Element into the set; copies the item.
    /// Return true if the item was not already in the set,
    /// else return false.
    bool insert(const Element& item)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();
        size_t before = the_set.size();
        the_set.insert(item);
        size_t after = the_set.size();
        lock.unlock();
        the_cond.notify_one();
        return before < after;
    }

    /// Insert the Element into the set, by moving it.
    /// Return true if the item was not already in the set,
    /// else return false.
    bool insert(Element&& item)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();
        size_t before = the_set.size();
        the_set.insert(std::move(item));
        size_t after = the_set.size();
        lock.unlock();
        the_cond.notify_one();
        return before < after;
    }

    /// Return true if the set is empty at this instant in time.
    /// Since other threads may have inserted or removed immediately
    /// after this call, the emptiness of the set may have
    /// changed by the time the caller looks at it.
    bool is_empty() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();
        return the_set.empty();
    }

    /// The set is unbounded. It will never get full.
    bool is_full() const noexcept { return false; }

    /// Return the size of the set at this instant in time.
    /// Since other threads may have inserted or removed immediately
    /// after this call, the size may have become incorrect by
    /// the time the caller looks at it.
    unsigned int size() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_set.size();
    }

    /// Try to get an element in the set. Return true if success,
    /// else return false. The element is removed from the set.
    bool try_get(Element& value)
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();
        if (the_set.empty())
        {
            return false;
        }

        auto it = the_set.begin();
        value = *it;
        the_set.erase(it);
        return true;
    }

    /// Get an item from the set. Block if the set is empty.
    /// The element is removed from the set, before this returns.
    void get(Element& value)
    {
        std::unique_lock<std::mutex> lock(the_mutex);

        // Use two nested loops here.  It can happen that the cond
        // wakes up, and yet the set is empty.
        do
        {
            while (the_set.empty() and not is_canceled)
            {
                the_cond.wait(lock);
            }
            if (is_canceled) throw Canceled();
        }
        while (the_set.empty());

        auto it = the_set.begin();
        value = *it;
        the_set.erase(it);
    }
    void wait_get(Element& value) { get(value); }

    Element value_get()
    {
        Element value;
        get(value);
        return value;
    }

    std::set<Element> wait_and_take_all()
    {
        std::unique_lock<std::mutex> lock(the_mutex);

        // Use two nested loops here.  It can happen that the cond
        // wakes up, and yet the set is empty.
        do
        {
            while (the_set.empty() and not is_canceled)
            {
                the_cond.wait(lock);
            }
            if (is_canceled) break;
        }
        while (the_set.empty());

        std::set<Element> retval;
        std::swap(retval, the_set);
        return retval;
    }

    /// A weak barrier.  This will block as long as the set is empty,
    /// returning only when the set isn't. It's "weak", because while
    /// it waits, other threads may insert and then remove something from
    /// the set, while this thread slept the entire time. However,
    /// if this call does return, then the set is almost surely not
    /// empty.  "Almost surely" means that none of the other threads
    /// that are currently waiting to get from the set will be woken.
    void barrier()
    {
        std::unique_lock<std::mutex> lock(the_mutex);

        while (the_set.empty() and not is_canceled)
        {
            the_cond.wait(lock);
        }
        if (is_canceled) throw Canceled();
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
    }
    void close() { cancel(); }

    bool is_closed() const noexcept { return is_canceled; }

    static bool is_lock_free() noexcept { return false; }
};
/** @}*/

#endif // __CONCURRENT_SET__
