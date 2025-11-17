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

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <set>
#include <vector>

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
///
/// The Compare template parameter specifies the comparison function to use
/// for ordering elements in the set. It defaults to std::less<Element>.

template<typename Element, typename Compare = std::less<Element>>
class concurrent_set
{
private:
    std::set<Element, Compare> the_set;
    mutable std::mutex the_mutex;
    std::condition_variable the_cond;
    std::condition_variable _watermark_cond;
    bool is_canceled;
    size_t _high_watermark;
    size_t _low_watermark;
    std::atomic<size_t> _blocked_inserters;

    concurrent_set(const concurrent_set&) = delete;  // disable copying
    concurrent_set& operator=(const concurrent_set&) = delete; // no assign

public:
    concurrent_set(void)
        : the_set(), the_mutex(), the_cond(), _watermark_cond(),
          is_canceled(false),
          _high_watermark(DEFAULT_HIGH_WATER_MARK),
          _low_watermark(DEFAULT_LOW_WATER_MARK),
          _blocked_inserters(0)
    {}
    concurrent_set(const Compare& comp)
        : the_set(comp), the_mutex(), the_cond(), _watermark_cond(),
          is_canceled(false),
          _high_watermark(DEFAULT_HIGH_WATER_MARK),
          _low_watermark(DEFAULT_LOW_WATER_MARK),
          _blocked_inserters(0)
    {}
    ~concurrent_set()
    { if (not is_canceled) cancel(); }

    struct Canceled : public std::exception
    {
        const char * what() { return "Cancellation of wait on concurrent_set"; }
    };

    // These limits seem ... reasonable ...
    static constexpr size_t DEFAULT_HIGH_WATER_MARK = INT32_MAX;
    static constexpr size_t DEFAULT_LOW_WATER_MARK = INT32_MAX - 65536;

private:
    /// Insert the Element into the set.
    /// Return true if the item was not already in the set,
    /// else return false.
    template<typename InsertFunc>
    bool insert_impl(InsertFunc&& do_insert)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        if (is_canceled) throw Canceled();

        // Block if set is at or above high watermark
        bool was_blocked = false;
        if (the_set.size() >= _high_watermark)
        {
            was_blocked = true;
            _blocked_inserters++;
            while (the_set.size() >= _high_watermark and not is_canceled)
            {
                _watermark_cond.wait(lock);
            }
            _blocked_inserters--;
            if (is_canceled) throw Canceled();
        }

        size_t before = the_set.size();
        do_insert();
        size_t after = the_set.size();

        // If this inserter was blocked and there are still blocked
        // inserters waiting, notify them all so they can check if
        // there's room.
        bool should_cascade = (was_blocked and _blocked_inserters > 0);

        lock.unlock();
        the_cond.notify_one();

        if (should_cascade)
            _watermark_cond.notify_all();

        return before < after;
    }

public:
    bool insert(const Element& item)
    {
        return insert_impl([&]() { the_set.insert(item); });
    }
    bool insert(Element&& item)
    {
        return insert_impl([&]() { the_set.insert(std::move(item)); });
    }

    /// Remove the Element from the set. Return number of Elements
    /// removed, i.e. 1 or 0, depending on whether the item was found
    /// (or not) in the set.
    size_t erase(const Element& item)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        return the_set.erase(item);
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

    /// Return true if the set is at/above high watermark or has
    /// blocked inserters.
    bool is_full() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_set.size() >= _high_watermark or _blocked_inserters > 0;
    }

    /// Return the size of the set at this instant in time.
    /// Since other threads may have inserted or removed immediately
    /// after this call, the size may have become incorrect by
    /// the time the caller looks at it.
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_set.size();
    }

    std::set<Element, Compare> peek() const
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        std::set<Element, Compare> copy(the_set);
        return copy;
    }

    /// Erase all elements from the container.
    void clear()
    {
        std::lock_guard<std::mutex> lock(the_mutex);
        return the_set.clear();
    }

#define COMMON_WATERMARK_NOTIFY {                         \
        /* Wake up waiting pushers when dropping below */ \
        /* low watermark. (hysteresis)                 */ \
        bool should_notify = (_blocked_inserters > 0) and \
                             (the_set.size() < _low_watermark); \
        lock.unlock();                                    \
        if (should_notify)                                \
            _watermark_cond.notify_all(); }

    /// Try to get an element in the set. Return true if success,
    /// else return false. The element is removed from the set.
    /// The reverse flag, if set, returns an element from the end
    /// of the set. Since elements of the set are ordered, sampling
    /// from both ends helps prevent stagnant elements accumulating
    /// at the end of the set.
    ///
    /// This works even if the set is closed, and can therefore be
    /// used to drain a closed set.
    bool try_get(Element& value, bool reverse = false)
    {
        std::unique_lock<std::mutex> lock(the_mutex);
        if (the_set.empty())
            return false;

        if (reverse)
        {
            // Frick-n-frack, cannot cast reverse iterators to
            // forward iterators. Also, there is no erase() that
            // accepts a reverse iterator. This means that
            // fetches from the end actually run slower. Dang.
            typename std::set<Element>::const_reverse_iterator it = the_set.crbegin();
            value = *it;
            the_set.erase(value);
        }
        else
        {
            typename std::set <Element>::const_iterator it = the_set.cbegin();
            value = *it;
            the_set.erase(it);
        }

        COMMON_WATERMARK_NOTIFY
        return true;
    }

    /// Same as above, but tries to get at most `nelt` of them. If there
    /// are fewer, then fewer are returned. The goal here is to reduce the
    /// number of locks taken.
    std::vector<Element> try_get(size_t nelt, bool reverse = false)
    {
        std::vector<Element> elvec;

        std::unique_lock<std::mutex> lock(the_mutex);
        if (the_set.empty())
            return elvec;

        if (the_set.size() < nelt) nelt = the_set.size();

        if (reverse)
        {
            for (size_t j=0; j<nelt; j++)
            {
                // Frick-n-frack, cannot cast reverse iterators to
                // forward iterators. Also, there is no erase() that
                // accepts a reverse iterator. This means that
                // fetches from the end actually run slower. Dang.
                typename std::set<Element>::const_reverse_iterator it = the_set.crbegin();
                Element value = *it;
                the_set.erase(value);
                elvec.emplace_back(value);
            }
        }
        else
        {
            for (size_t j=0; j<nelt; j++)
            {
                typename std::set <Element>::const_iterator it = the_set.cbegin();
                Element value = *it;
                the_set.erase(it);
                elvec.emplace_back(value);
            }
        }

        COMMON_WATERMARK_NOTIFY
        return elvec;
    }

#define COMMON_COND_WAIT(DO_THING)                           \
        std::unique_lock<std::mutex> lock(the_mutex);        \
        /* Use two nested loops here.  It can happen that */ \
        /* the cond wakes up, and yet the queue is empty. */ \
        do {                                                 \
            while (the_set.empty() and not is_canceled)      \
                the_cond.wait(lock);                         \
            if (is_canceled) DO_THING;                       \
        } while (the_set.empty());

    /// Get an item from the set. Block if the set is empty.
    /// The element is removed from the set, before this returns.
    void get(Element& value)
    {
        COMMON_COND_WAIT({ throw Canceled(); })

        auto it = the_set.begin();
        value = *it;
        the_set.erase(it);

        COMMON_WATERMARK_NOTIFY
    }
    void wait_get(Element& value) { get(value); }

#undef COMMON_WATERMARK_NOTIFY

    Element value_get()
    {
        Element value;
        get(value);
        return value;
    }

    std::set<Element, Compare> wait_and_take_all()
    {
        COMMON_COND_WAIT({ break; })

        std::set<Element, Compare> retval(the_set.key_comp());
        std::swap(retval, the_set);
        return retval;
    }
#undef COMMON_COND_WAIT

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

    /// Set the high and low watermarks for the set.
    /// When the set size reaches or exceeds the high watermark,
    /// insert() operations will block until the size drops below
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

#endif // __CONCURRENT_SET__
