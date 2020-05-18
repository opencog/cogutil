/*
 * util/pool.h ---
 *
 * Copyright (C) 2012 Poulin Holdings LLC
 *
 * Author: Linas Vepstas
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


#ifndef _OPENCOG_UTIL_POOL_H
#define _OPENCOG_UTIL_POOL_H

#include <queue>
#include <mutex>
#include <condition_variable>

namespace opencog {
/** \addtogroup grp_cogutil
 *  @{
 */

//! Thread-safe blocking resource allocator.
/// If there are no resources to borrow, then the borrow() method will
/// block until one is given back.
///
/// This maintains a finite-sized pool of resources, which can be
/// borrowed and returned, much like a library book.  As long as the
/// pool is not empty, resources can be borrowed. If the pool is empty,
/// (there is nothing to be borrowed), then the borrowing thread will
/// block, until such time as the pool becomes non-empty.  When a
/// resource is returned, one of the blocked threads will be woken up,
/// and given the resource. Wakeups are in arbitrary order, and not
/// in any particular order (the thread waiting the longest is not
/// necessarily the one that gets woken up).
///
/// To start things off, resources need to be added to the pool.  This
/// can be done at any time at all: even after borrowers are lined up.
/// Resources can be permanently removed, simply by borrowing them and
/// never returning them.
///
/// A typical usage would be to create the pool in some master thread,
/// call `give_back()` N times to place N items in the pool, and then
/// let other threads start borrowing.  Alternately, other threads
/// can start borrowing right away; they'll block until someone puts
/// something into the pool.
///
/// Although you might think that boost::pool already does this, you'd
/// be wrong.  It would appear that the author of boost::pool did not
/// understand the problem that needed to be solved.
///
/// This is similar to concurrent_queue (also in this directory) but
/// with a simpler, less sophisticated API.  The main difference is that
/// this API uses names that make it clear that resources are being
/// borrowed, and returned, whereas the concurrent_queue has an API that
/// is geared towards the producer-consumer mode of thinking.
//
// This is implemented on top of std::queue, for no particular reason;
// pretty much any other container would do.
//
template<typename Resource>
class pool
{
    public:
        /// Fetch a resource from the pool. Block if the pool is empty.
        /// If blocked, this will unblock when a resource is put into
        /// the pool.
        Resource& borrow()
        {
            std::unique_lock<std::mutex> lock(mu);
            while (objs.empty()) {
                cond.wait(lock);
            }
            Resource& rv = objs.front();
            objs.pop();
            return rv;
        }

        /// Put a resource into the pool.  If the pool is empty, and
        /// other threads are blocked and waiting, this will release
        /// some other blocked thread.
        void give_back(const Resource& obj)
        {
            std::lock_guard<std::mutex> lock(mu);
            objs.push(obj);
            cond.notify_one();
        }

        size_t available()
        {
            return objs.size();
        }

    private:
        std::mutex mu;
        std::condition_variable cond;
        std::queue<Resource> objs;
};


/** @}*/
} // ~namespace opencog

#endif // _OPENCOG_UTIL_POOL_H
