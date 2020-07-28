/*
 * FUNCTION:
 * Multi-threaded asynchronous write buffer.
 *
 * HISTORY:
 * Copyright (c) 2013, 2015, 2017, 2020 Linas Vepstas <linasvepstas@gmail.com>
 *
 * LICENSE:
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

#ifndef _OC_ASYNC_BUFFER_H
#define _OC_ASYNC_BUFFER_H

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include <opencog/util/concurrent_set.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>
#include <opencog/util/macros.h>

namespace opencog
{
/** \addtogroup grp_cogutil
 *  @{
 */

/**
 * Thread-safe, multi-threaded asynchronous write buffer.
 *
 * This class provides a simple way to write de-duplicated data,
 * asynchornously. It is similar to the async_caller class, in that
 * it calls the indicated method in some different thread, at some
 * later time. Unlike the async caller, it de-duplicates the data
 * before making the call.  That is, if the call is queued multiple
 * times with the same data, if will be called only once, for that
 * data item.
 *
 * This de-duplication can be very handy if the method is writing data,
 * and the caller is repeatedly asking for the same data to be written.
 * By bufferring up and de-duplicating the write requests, this class
 * can reduce load on the system, especially if a delay is introduced,
 * so that the requests buffer up before being serviced.
 *
 * By default, this class attempts to service requests as soon as they
 * are queued up. However, if the stall() flag is set, then no writing
 * will be done, until at least the low_watermark amount of work
 * accumulates. This can be useful to force the de-duplication mechanism
 * to do it's work: if forces a minimum number of unique elements to be
 * queued up before processing starts.
 *
 * In other respects, it provides the same advantages that the
 * async_caller class does: The buffering helps, if each call
 * takes a long time to run, or if it blocks waiting on I/O.  By
 * running in a different thread, it allows the current thread to
 * return immediately.  It also enables concurrency: a large number of
 * threads can handle the time-consuming work in parallel, while the
 * master thread can zip along.
 *
 * You'd think that there would be some BOOST function for this, but
 * there doesn't seem to be ...
 *
 * This class allows a simple implementation of a thread-safe, multi-
 * threaded de-duplicating write buffer. It is currently used by
 * several different storage classes, to write atoms out to disk,
 * or to do async network I/O.
 *
 * What actually happens is this: The given elements are placed in a
 * set (in a thread-safe manner -- the enqueue function can be safely
 * called from multiple threads.) This set is then serviced and
 * drained by a pool of active threads, which remove the elements, and
 * call the method on each one.
 *
 * The implementation is very simple: it uses a fixed-size thread pool.
 * It uses hi/lo watermarks to stall and drain the set, if it gets too
 * large. This could probably be made spiffier.
 *
 * It would also be clearer to the user, if we placed the method to call
 * into the set, along with the element, instead of specifying the
 * method in the ctor. This would really drive home the point that this
 * really is just an async method call. XXX TODO FIXME someday.
 *
 * The number of threads to use for writing is fixed, when the ctor is
 * called.  The default is 4 threads.  This can be set to zero, if
 * desired; as a result, all writes will be *synchronous*.  This can
 * be a useful thing to do, if this class is being used in a temporary
 * instance somewhere, and the overhead of creating threads is to be
 * avoided. (For example, temporary AtomTables used during evaluation.)
 *
 * Setting the number of threads equal to the number of hardware cores
 * is probably a bad idea; there are situations where this seems to slow
 * the system down.
 *
 * Issues: This is a good but minimalist implementation, and thus has a
 * few issues.  One is that the barrier() call is stronger than it needs
 * to be, and thus can deadlock if called within a writer. All that the
 * barrier() call needs to do is to be a fence, ensuring that everything
 * before really is before everything after. It didn't need to actually
 * drain everything.
 *
 * Another issue is that the assorted loops to drain queues use
 * spin-loops and usleeps. I think this is mostly harmless, but is
 * impure, and should be replaced by CV's. Doing so becomes much easier
 * once C++20 is widely available, as it has CV's on std::atomic ints.
 */
template<typename Writer, typename Element>
class async_buffer
{
	private:
		concurrent_set<Element> _store_set;
		std::vector<std::thread> _write_threads;
		std::mutex _write_mutex;
		std::mutex _enqueue_mutex;
		std::atomic<unsigned long> _busy_writers;
		std::atomic<unsigned long> _pending;
		size_t _high_watermark;
		size_t _low_watermark;

		Writer* _writer;
		void (Writer::*_do_write)(const Element&);

		unsigned int _thread_count;
		bool _stopping_writers;

		bool _stall_writers;

		void start_writer_thread();
		void stop_writer_threads();
		void write_loop();

		void do_insert(const Element&);
		void drain();

	public:
		async_buffer(Writer*, void (Writer::*)(const Element&), int nthreads=4);
		~async_buffer();
		void insert(const Element&);
		void flush();
		void barrier();

		void set_watermarks(size_t, size_t);
		void stall(bool);

		// Utilities for monitoring performance.
		// _item_count == number of attempted insertions;
		// _duplicate_count == number of duplicates dropped.
		// _drain_count == number of times the high watermark was hit.
		// _drain_msec == accumulated number of millisecs to drain.
		// _drain_concurrent == number of threads that hit queue-full.
		bool _in_drain;
		std::atomic<unsigned long> _item_count;
		std::atomic<unsigned long> _duplicate_count;
		std::atomic<unsigned long> _flush_count;
		std::atomic<unsigned long> _drain_count;
		std::atomic<unsigned long> _drain_msec;
		std::atomic<unsigned long> _drain_slowest_msec;
		std::atomic<unsigned long> _drain_concurrent;

		unsigned long get_busy_writers() const { return _busy_writers; }
		unsigned long get_size() const { return _pending; }
		unsigned long get_high_watermark() const { return _high_watermark; }
		unsigned long get_low_watermark() const { return _low_watermark; }
		bool stalling() const { return _stall_writers; }

		void clear_stats();
};



/* ================================================================ */
// Constructors

#define DEFAULT_HIGH_WATER_MARK 100
#define DEFAULT_LOW_WATER_MARK 10

/// Writer: the class whose method will be called.
/// cb: the method that will be called.
/// nthreads: the number of threads in the writer pool to use. Defaults
/// to 4 if not specified.
template<typename Writer, typename Element>
async_buffer<Writer, Element>::async_buffer(Writer* wr,
                                            void (Writer::*cb)(const Element&),
                                            int nthreads)
{
	_writer = wr;
	_do_write = cb;
	_stopping_writers = false;
	_thread_count = 0;
	_busy_writers = 0;
	_pending = 0;
	_stall_writers = false;
	_in_drain = false;

	_high_watermark = DEFAULT_HIGH_WATER_MARK;
	_low_watermark = DEFAULT_LOW_WATER_MARK;

	clear_stats();

	for (int i=0; i<nthreads; i++)
	{
		start_writer_thread();
	}
}

template<typename Writer, typename Element>
async_buffer<Writer, Element>::~async_buffer()
{
	stop_writer_threads();
}

/// Set the high and low watermarks for processing. These are useful
/// for preventing excessive backlogs of unprocessed elements from
/// accumulating. When enqueueing new work, any threads that encounter
/// an unprocessed backlog exceeding the high watermark will block
/// until the backlog drops below the low watermark.  Note that if
/// some threads are blocked, waiting for this drain to occur, that
/// this does not prevent other threads from adding more work, as long
/// as those other threads did not see a large backlog.
///
/// If write-stalling is enabled, then no writing will be done until
/// at least the low_watermark number of elements have accumulated.
///
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::set_watermarks(size_t hi, size_t lo)
{
	_high_watermark = hi;
	_low_watermark = lo;
}

/// Intentionally stall the writer threads, prevent them from writing
/// until at least _low_watermark elements have accumulated in the pool.
/// The goal here is to allow the de-duplication services to actually
/// do thier work.  This has the dangerous side-effect of potentially
/// leaving eleemnts in the set forever, never quite getting them
/// written out. Caveat emptor! You may want to flush periodically,
/// to avoid this situation.
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::stall(bool st)
{
	_stall_writers = st;
}

template<typename Writer, typename Element>
void async_buffer<Writer, Element>::clear_stats()
{
	_item_count = 0;
	_duplicate_count = 0;
	_flush_count = 0;
	_drain_count = 0;
	_drain_msec = 0;
	_drain_slowest_msec = 0;
	_drain_concurrent = 0;
}

/* ================================================================ */

/// Start a single writer thread.
/// May be called multiple times.
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::start_writer_thread()
{
	// logger().info("async_buffer: starting a writer thread");
	std::unique_lock<std::mutex> lock(_write_mutex);
	if (_stopping_writers)
		throw RuntimeException(TRACE_INFO,
			"Cannot start; async_buffer writer threads are being stopped!");

	_write_threads.push_back(std::thread(&async_buffer::write_loop, this));
	_thread_count ++;
}

/// Stop all writer threads, but only after they are done writing.
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::stop_writer_threads()
{
	_stall_writers = false;

	// logger().info("async_buffer: stopping all writer threads");
	std::unique_lock<std::mutex> lock(_write_mutex);
	_stopping_writers = true;

	// Spin a while, until the writer threads are (mostly) done.
	while (0 < _pending)
	{
		// std::this_thread::sleep_for(std::chrono::milliseconds(1));
		usleep(1000);
	}

	// Now tell all the threads that they are done.
	// I.e. cancel all the threads.
	_store_set.cancel();
	while (0 < _write_threads.size())
	{
		_write_threads.back().join();
		_write_threads.pop_back();
		_thread_count --;
	}

	// OK, so we've joined all the threads, but the set
	// might not be totally empty; some dregs might remain.
	// Drain it now, single-threadedly.
	_store_set.cancel_reset();
	while (not _store_set.is_empty())
	{
		Element elt = _store_set.value_get();
		(_writer->*_do_write)(elt);
	}
	
	// Its now OK to start new threads, if desired ...(!)
	_stopping_writers = false;
}


/// Drain the set.  Non-synchronizing.
///
/// This will deadlock, if called from a writer thread.
/// Thus, not for public use.
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::drain()
{
	bool save_stall = _stall_writers;
	_stall_writers = false;
	_flush_count++;

	// XXX TODO - when C++20 becomes widely available,
	// replace this loop by _pending.wait(0)
	while (0 < _pending)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		// usleep(1000);
	}

	_stall_writers = save_stall;
}

/// Drain the set.  Non-synchronizing.
///
/// This is NOT synchronizing! It does NOT prevent other threads from
/// concurrently adding to the queue! Thus, if these other threads are
/// adding at a high rate, this call might not return for a long time;
/// it might never return! There is no guarantee of forward progress!
///
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::flush()
{
	bool save_stall = _stall_writers;
	_stall_writers = false;
	_flush_count++;

	while (0 < _store_set.size())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		// usleep(1000);
	}

	_stall_writers = save_stall;
}

/// Drain the set.  Synchronizing.
///
/// This forces a drain of the pending work-set. It prevents other
/// threads from adding to the set, while the drain is being performed.
/// It will wait not only for the pending work-queue to empty, but also
/// for all writers to have completed.
///
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::barrier()
{
	std::unique_lock<std::mutex> lock(_enqueue_mutex);

	// We cannot poll _pending in a writer thread, as it will never
	// drop to zero, resulting in a deadlock.
	std::thread::id tid = std::this_thread::get_id();
	for (const auto& th : _write_threads)
	{
		if (th.get_id() == tid)
		{
			flush();
			return;
		}
	}

	drain();
}

/// A single write thread. Reads elements from set, and invokes the
/// method on them.
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::write_loop()
{
	try
	{
		while (true)
		{
			// Do nothing, if asked to stall.
			while (_stall_writers and _store_set.size() < _low_watermark)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
			}

			Element elt = _store_set.value_get();
			_busy_writers ++;
			(_writer->*_do_write)(elt);
			_busy_writers --;
			_pending --;
		}
	}
	catch (typename concurrent_set<Element>::Canceled& e)
	{
		// We are so out of here. Nothing to do, just exit this thread.
		return;
	}
}


/* ================================================================ */

/// Insert, no matter what. Private, unsafe for external use.
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::do_insert(const Element& elt)
{
	_pending ++;
	bool inserted = _store_set.insert(elt);
	_item_count++;
	if (not inserted)
	{
		_duplicate_count++;
		_pending --;
	}
}

/**
 * Insert the given element.  Returns immediately after inserting.
 * Thread-safe: this may be called concurrently from multiple threads.
 * If the set is over-full, then this will block until the set is
 * mostly drained...
 */
template<typename Writer, typename Element>
void async_buffer<Writer, Element>::insert(const Element& elt)
{
	// Sanity checks.
	if (_stopping_writers)
		throw RuntimeException(TRACE_INFO,
			"Cannot store; async_buffer writer threads are being stopped!");

	if (0 == _thread_count)
	{
		// If there are no async writer threads, then silently perform
		// a synchronous write.  This situation happens if this class
		// was constructed with zero writer threads.  The user may want
		// to do this if this class is being used in a temporary,
		// transient object, and the user wants to avoid the overhead
		// of creating threads.
		_item_count++;
		(_writer->*_do_write)(elt);
		return;
	}

	// Must not honor the enqueue mutex when in a writer thread,
	// as otherwise a deadlock will result.
	bool need_insert = true;
	std::thread::id tid = std::this_thread::get_id();
	for (const auto& th : _write_threads)
	{
		if (th.get_id() == tid)
		{
			do_insert(elt);
			need_insert = false;
			break;
		}
	}

	// The _store_set.insert(elt) does not need a lock, itself; its
	// perfectly thread-safe. However, the flush barrier does need to
	// be able to halt everyone else from enqueing more stuff, so we
	// do need to use a lock for that.
	if (need_insert)
	{
		std::unique_lock<std::mutex> lock(_enqueue_mutex);
		do_insert(elt);
	}

	// If the writer threads are falling behind, mitigate.
	// Right now, this will be real simple: just spin and wait
	// for things to catch up.  Maybe we should launch more threads!?
	// Note also: even as we block this thread, waiting for the drain
	// to complete, other threads might be filling the set back up.
	// If it does over-fill, then those threads will also block, one
	// by one, until we hit a metastable state, where the active
	// (non-stalled) fillers and emptiers are in balance.

	if (_high_watermark < _store_set.size())
	{
		if (_in_drain) _drain_concurrent ++;
		else _drain_count++;

		_in_drain = true;
		// unsigned long cnt = 0;
		auto start = std::chrono::steady_clock::now();
		do
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			// usleep(1000);
			// cnt++;
		}
		while (_low_watermark < _store_set.size());
		_in_drain = false;

		// Sleep might not be accurate, so measure elapsed time directly.
		auto end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		unsigned long msec = duration.count();

		logger().debug("async_buffer overfull set; had to sleep %d millisecs to drain!", msec);
		_drain_msec += msec;
		if (_drain_slowest_msec < msec) _drain_slowest_msec = msec;
	}
}

/** @}*/
} // namespace opencog

#endif // _OC_ASYNC_BUFFER_H
