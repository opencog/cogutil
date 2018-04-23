/*
 * ThreadLocal.h
 *
 * Author: Vitaly Bogdanov
 *
 * Copyright (C) 2018 Vitaly Bogdanov <vitaly@singularitynet.io>
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
#ifndef OPENCOG_QUERY_THREADLOCAL_H_
#define OPENCOG_QUERY_THREADLOCAL_H_

#include <memory>
#include <unordered_map>
#include <functional>
#include <thread>
#include <boost/thread.hpp>

namespace opencog
{

/**
 * Utility class to instantiate, hold, return and destroy thread-specific
 * instances of class T.
 */
template <class T>
class ThreadLocal
{
    /** Map to keep thread-specific values by std::thread::id */
    std::unordered_map<std::thread::id, std::unique_ptr<T>> value_by_thread_id;
    /** Mutex is locked in shared mode to get values and in exclusive mode
     * to write values */
    boost::shared_mutex read_write_mutex;

    /**
     * Callable to instantiate thread specific value
     * @return pointer to created instance
     */
    std::function<T*()> value_constructor;

public:

    /**
     * ThreadLocal constructor
     * @param value_constructor callable to return new value instance
     */
    ThreadLocal(std::function<T*()> value_constructor) :
        value_constructor(value_constructor)
    {}

    ThreadLocal(const ThreadLocal<T>& other) = delete;
    ThreadLocal(ThreadLocal<T>&& other) = delete;

    /**
     * Return value if it was already created for calling thread.
     * In other case create new instance and return it.
     * @return thread-specific instance of T
     */
    T& get()
    {
        std::thread::id thread_id = std::this_thread::get_id();

        {
            boost::shared_lock<boost::shared_mutex> read_lock(read_write_mutex);
            auto it = value_by_thread_id.find(thread_id);
            if (it != value_by_thread_id.end()) {
                return *(it->second);
            }
        }

        {
            boost::lock_guard<boost::shared_mutex> write_lock(read_write_mutex);
            value_by_thread_id.emplace(thread_id, value_constructor());
            return *(value_by_thread_id.at(thread_id));
        }
    }
};

} /* namespace opencog */

#endif /* OPENCOG_QUERY_THREADLOCAL_H_ */
