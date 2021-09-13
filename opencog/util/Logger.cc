/*
 * opencog/util/Logger.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008, 2010 OpenCog Foundation
 * Copyright (C) 2009, 2011, 2013 Linas Vepstas
 * All Rights Reserved
 *
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 *            Linas Vepstas <linasvepstas@gmail.com>
 *            Joel Pitt <joel@opencog.org>
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

#if defined(HAVE_GNU_BACKTRACE)
#include <cxxabi.h>
#include <execinfo.h>
#endif

#include <iostream>
#include <sstream>

#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>


#ifdef WIN32_NOT_UNIX
#include <winsock2.h>
#undef ERROR
#undef DEBUG
#else
#include <sys/time.h>
#endif

#ifdef HAVE_VALGRIND
#include <valgrind/drd.h>
#endif

#include <opencog/util/backtrace-symbols.h>
#include <opencog/util/platform.h>

#include "Logger.h"

#ifdef __APPLE__
#define fdatasync fsync
#endif

using namespace opencog;

// messages greater than this will be truncated
#define MAX_PRINTF_STYLE_MESSAGE_SIZE (1<<15)
const char* levelStrings[] = {"NONE", "ERROR", "WARN", "INFO", "DEBUG", "FINE"};

#if defined(HAVE_GNU_BACKTRACE) /// @todo backtrace and backtrace_symbols
                                /// is LINUX, we may need a WIN32 version
static void prt_backtrace(std::ostringstream& oss)
{
#define BT_BUFSZ 50
	void *bt_buf[BT_BUFSZ];

	int stack_depth = backtrace(bt_buf, BT_BUFSZ);
	char **syms = oc_backtrace_symbols(bt_buf, stack_depth);

    // Depending on how the dependencies are met, syms could be NULL
    if (syms == NULL) return;

	// Start printing at a bit into the stack, so as to avoid recording
	// the logger functions in the stack trace.
	oss << "\tStack Trace:\n";
	for (int i=2; i < stack_depth; i++)
	{
		// Most things we'll print are mangled C++ names,
		// So demangle them, get them to pretty-print.
#if defined(HAVE_BFD) && defined(HAVE_IBERTY)
		// The standard and the heck versions differ slightly in layout.
		char * begin = strstr(syms[i], "_ZN");
		char * end = strchr(syms[i], '(');
		if (!(begin && end) || end <= begin)
		{
			// Failed to pull apart the symbol names
			oss << "\t" << i << ": " << syms[i] << "\n";
		}
		else
		{
			*begin = 0x0;
			oss << "\t" << i << ": " << syms[i] << "  ";
			*begin = '_';
			size_t sz = 250;
			int status;
			char *fname = (char *) malloc(sz);
			*end = 0x0;
			char *rv = abi::__cxa_demangle(begin, fname, &sz, &status);
			if (rv) fname = rv; // might have re-alloced
			oss << fname << std::endl;
			free(fname);
		}
#else
		char * begin = strchr(syms[i], '(');
		char * end = strchr(syms[i], '+');
		if (!(begin && end) || end <= begin)
		{
			// Failed to pull apart the symbol names
			oss << "\t" << i << ": " << syms[i] << "\n";
		}
		else
		{
			*begin = 0x0;
			oss << "\t" << i << ": " << syms[i] << " ";
			*begin = '(';
			size_t sz = 250;
			int status;
			char *fname = (char *) malloc(sz);
			*end = 0x0;
			char *rv = abi::__cxa_demangle(begin+1, fname, &sz, &status);
			*end = '+';
			if (rv) fname = rv; // might have re-alloced
			oss << "(" << fname << " " << end << std::endl;
			free(fname);
		}
#endif
	}
	oss << std::endl;
	free(syms);
}
#endif

std::mutex Logger::_loggers_mtx;
std::map<std::string, Logger::LogWriter*> Logger::_loggers;

Logger::~Logger()
{
    // Do NOT destroy/delete the LogWriter; other logger instances
    // might be using it! Do not touch it; it may no longer exist.
    _log_writer = nullptr;
}

Logger::LogWriter::LogWriter(void)
{
    writingLoopActive = false;
#ifdef HAVE_VALGRIND
    DRD_IGNORE_VAR(this->msg_queue);
#endif
    logfile = NULL;
    pending_write = false;
}

Logger::LogWriter::~LogWriter()
{
    if (logfile == NULL) return;

    // Remove the logfile from the list.
    std::lock_guard<std::mutex> lock(_loggers_mtx);
    _loggers.erase(fileName);

    // Wait for queue to empty
    flush();
    stop_write_loop();
    fflush(logfile);
    fclose(logfile);
    logfile = nullptr;
}

void Logger::LogWriter::start_write_loop()
{
    std::unique_lock<std::mutex> lock(the_mutex);
    if (!writingLoopActive)
        writer_thread = std::thread(&Logger::LogWriter::writing_loop, this);
}

void Logger::LogWriter::stop_write_loop()
{
    std::unique_lock<std::mutex> lock(the_mutex);
    msg_queue.close();
    // rejoin thread
    writer_thread.join();
}

void Logger::LogWriter::writing_loop()
{
    set_thread_name("opencog:logger");

    // When the thread exits, make sure that all pending messages have
    // been written to the logfile. This code is here because the usual
    // case is that the singleton logger() has continued to live on
    // until program termination.  If it was ever used, a LogWriter
    // thread was started (this thread), and the ~LogWriter() dtor
    // never gets a chance to run before program exit. We cannot call
    // ~LogWriter() in the shared library _fini() because, by then,
    // other assorted globals may be gone, and, in particular, this
    // thread might have been destructed already. So our one and only
    // chance to flush the message queue is on thread exit, namely
    // with the class below, whose dtor is called upon exit from scope.
    class OnExit
    {
        public:
            LogWriter* that;
            ~OnExit()
            {
                // Try very very hard to make sure that the message
                // queue has been completely drained.
                while (not that->msg_queue.is_closed())
                {
                    std::string* msg = that->msg_queue.value_pop();
                    that->write_msg(*msg);
                    delete msg;
                }
                that->pending_write = false;
                that->writingLoopActive = false;
                fflush(that->logfile);
                fclose(that->logfile);
                that->logfile = nullptr;
            }
    };
    thread_local OnExit exiter;
    exiter.that = this;

    writingLoopActive = true;
    try
    {
        while (true)
        {
            // The pending_write flag prevents Logger::flush()
            // from returning prematurely.
            std::string* msg = msg_queue.value_pop();
            pending_write = true;
            write_msg(*msg);
            pending_write = false;
            delete msg;
        }
    }
    catch (concurrent_queue< std::string* >::Canceled &e)
    {
    }
    pending_write = false;
    writingLoopActive = false;
}

void Logger::flush()
{
    if (_log_writer) _log_writer->flush();
}

void Logger::LogWriter::flush()
{
    // There is a timing window between when pending_write is set,
    // and the msg_queue being empty. We could fall through that
    // window. Yes, its stupid, but too low-importance to fix.
    // try to work around it by sleeping.
    usleep(10);

    // Perhaps we could do this with semaphores, but this is not
    // really critical code, so a busy-wait is good enough.
    while (pending_write or not msg_queue.is_empty())
    {
        usleep(100);
    }

    // Force a write to the disk. Don't need to update metadata, though.
    if (logfile) fdatasync(fileno(logfile));
}

void Logger::LogWriter::write_msg(const std::string &msg)
{
    std::unique_lock<std::mutex> lock(the_mutex);

    // Delay opening the file until the first logging statement is issued;
    // this allows us to set the main logger's filename without creating
    // a useless log file with the default filename.
    if (logfile == NULL)
    {
        if ((logfile = fopen(fileName.c_str(), "a")) == NULL)
        {
            fprintf(stderr, "[ERROR] Unable to open log file \"%s\"\n",
                    fileName.c_str());
            lock.unlock();
            stop_write_loop();
            return;
        }
    }

    // Write to file.
    int rc = fprintf(logfile, "%s", msg.c_str());
    if ((int) msg.size() != rc)
    {
        fprintf(stderr,
            "[ERROR] failed write to logfile, rc=%d sz=%zu\n",
            rc, msg.size());
        exit(1);
    }

    // Flush, because log messages are important, especially if we
    // are about to crash. So we don't want to have these buffered up.
    rc = fflush(logfile);
    if (0 != rc)
    {
        int norr = errno;
        fprintf(stderr,
            "[ERROR] failed to flush logfile, rc=%d errno=%d %s\n",
            rc, norr, strerror(norr));
        exit(1);
    }
}

Logger::Logger(const std::string &fname, Logger::Level level, bool tsEnabled)
    : error(*this), warn(*this), info(*this), debug(*this), fine(*this)
{
    set_filename(fname);

    this->currentLevel = level;
    this->backTraceLevel = ERROR;

    this->timestampEnabled = tsEnabled;
    this->threadIdEnabled = false;
    this->printLevel = true;
    this->printToStdout = false;
    this->syncEnabled = false;

    this->logEnabled = true;
#ifdef HAVE_VALGRIND
    DRD_IGNORE_VAR(this->logEnabled);
#endif
}

Logger::Logger(const Logger& log)
    : error(*this), warn(*this), info(*this), debug(*this), fine(*this)
{
    set(log);
}

Logger& Logger::operator=(const Logger& log)
{
    this->set(log);
    return *this;
}

void Logger::set(const Logger& log)
{
    this->component.assign(log.component);
    this->currentLevel = log.currentLevel;
    this->printToStdout = log.printToStdout;
    this->printLevel = log.printLevel;
    this->backTraceLevel = log.backTraceLevel;
    this->timestampEnabled = log.timestampEnabled;
    this->threadIdEnabled = log.threadIdEnabled;
    this->syncEnabled = log.syncEnabled;
    this->logEnabled = log.logEnabled;
}

// ***********************************************/
// API

void Logger::set_level(Logger::Level newLevel)
{
    currentLevel = newLevel;
}

Logger::Level Logger::get_level() const
{
    return currentLevel;
}

void Logger::set_backtrace_level(Logger::Level newLevel)
{
    backTraceLevel = newLevel;
}

Logger::Level Logger::get_backtrace_level() const
{
    return backTraceLevel;
}

void Logger::LogWriter::setFileName(const std::string& s)
{
    fileName.assign(s);

    std::unique_lock<std::mutex> lock(the_mutex);
    if (logfile != NULL) {
        flush();
        fclose(logfile);
    }
    logfile = NULL;

    lock.unlock();
    start_write_loop();
}

void Logger::set_filename(const std::string& fname)
{
    std::lock_guard<std::mutex> lock(_loggers_mtx);
    try {
        // If there already is an existing writer for this file, just
        // switch to it. Note that other logger instances might also
        // be using this writer!
        _log_writer = _loggers.at(fname);
    }
    catch (...) {
        _log_writer = new LogWriter();
        _log_writer->setFileName(fname);
        _loggers.insert({fname, _log_writer});
    }

    enable();
}

const std::string& Logger::get_filename() const
{
    static std::string bad;
    if (nullptr == _log_writer) return bad;
    return _log_writer->getFileName();
}

void Logger::set_component(const std::string& c)
{
    component = c;
}

const std::string& Logger::get_component() const
{
    return component;
}

void Logger::set_timestamp_flag(bool flag)
{
    timestampEnabled = flag;
}

void Logger::set_thread_id_flag(bool flag)
{
    threadIdEnabled = flag;
}

bool Logger::get_thread_id_flag() const
{
    return threadIdEnabled;
}

void Logger::set_print_to_stdout_flag(bool flag)
{
    printToStdout = flag;
}

bool Logger::get_print_to_stdout_flag() const
{
    return printToStdout;
}

void Logger::set_print_level_flag(bool flag)
{
    printLevel = flag;
}

void Logger::set_sync_flag(bool flag)
{
    syncEnabled = flag;
}

void Logger::set_print_error_level_stdout()
{
    set_print_to_stdout_flag(true);
    set_level(Logger::ERROR);
}

void Logger::enable()
{
    logEnabled = true;
}

void Logger::disable()
{
    logEnabled = false;
}

void Logger::log(Logger::Level level, const std::string &txt)
{
    static const unsigned int max_queue_size_allowed = 1024;
    // Don't log if not enabled, or level is too low.
    if (!logEnabled) return;
    if (level > currentLevel) return;
    if (nullptr == _log_writer) return;

    std::ostringstream oss;
    if (timestampEnabled)
    {
        struct timeval stv;
        struct tm stm;
        char timestamp[64];
        char timestampStr[256];


        ::gettimeofday(&stv, NULL);
        time_t t = stv.tv_sec;
        gmtime_r(&t, &stm);
        strftime(timestamp, sizeof(timestamp), "%F %T", &stm);
        snprintf(timestampStr, sizeof(timestampStr),
                "[%s:%03ld] ",timestamp, (long)stv.tv_usec / 1000);
        oss << timestampStr;
    }

    if (printLevel)
        oss << "[" << get_level_string(level) << "] ";

    if (!component.empty())
        oss << "[" << component << "] ";

    if (threadIdEnabled)
        oss << "[thread-" << std::this_thread::get_id() << "] ";

    oss << txt << std::endl;

#if defined(HAVE_GNU_BACKTRACE)
    if (level <= backTraceLevel)
    {
        prt_backtrace(oss);
    }
#endif

    _log_writer->qmsg(oss.str());

    // If the queue gets too full, block until it's flushed to
    // file. This can sometimes happen, if some component is spewing
    // lots of debugging messages in a tight loop.
    if (_log_writer->size() > max_queue_size_allowed) flush();

    // Errors are associated with imminent crashes. Make sure that the
    // stack trace is written to disk *before* the crash happens! Yes,
    // this introduces latency and lag. Tough. Don't generate errors.
    if (level <= backTraceLevel) flush();

    if (syncEnabled) flush();

    // Write to stdout.
    if (printToStdout)
    {
        std::cout << oss.str();
        std::cout.flush();
    }
}

void Logger::backtrace()
{
    if (nullptr == _log_writer) return;

    static const unsigned int max_queue_size_allowed = 1024;
    std::ostringstream oss;

    #if defined(HAVE_GNU_BACKTRACE)
    prt_backtrace(oss);
    #endif

    _log_writer->qmsg(oss.str());

    // If the queue gets too full, block until it's flushed to file or
    // stdout. This can sometimes happen, if some component is spewing
    // lots of debugging messages in a tight loop.
    if (_log_writer->size() > max_queue_size_allowed) {
        flush();
    }
}

void Logger::logva(Logger::Level level, const char *fmt, va_list args)
{
    if (level <= currentLevel) {
        char buffer[MAX_PRINTF_STYLE_MESSAGE_SIZE];
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        std::string msg = buffer;
        log(level, msg);
    }
}

void Logger::log(Logger::Level level, const char *fmt, ...)
{
    va_list args; va_start(args, fmt); logva(level, fmt, args); va_end(args);
}

void Logger::Error::operator()(const char *fmt, ...)
{
    va_list args; va_start(args, fmt); logger.logva(ERROR, fmt, args); va_end(args);
}

void Logger::Warn::operator()(const char *fmt, ...)
{
    va_list args; va_start(args, fmt); logger.logva(WARN,  fmt, args); va_end(args);
}

void Logger::Info::operator()(const char *fmt, ...)
{
    va_list args; va_start(args, fmt); logger.logva(INFO,  fmt, args); va_end(args);
}

void Logger::Debug::operator()(const char *fmt, ...)
{
    va_list args; va_start(args, fmt); logger.logva(DEBUG, fmt, args); va_end(args);
}

void Logger::Fine::operator()(const char *fmt, ...)
{
    va_list args; va_start(args, fmt); logger.logva(FINE,  fmt, args); va_end(args);
}

const char* Logger::get_level_string(const Logger::Level level)
{
    if (level == BAD_LEVEL)
        return "Bad level";
    else
        return levelStrings[level];
}

Logger::Level Logger::get_level_from_string(const std::string& levelStr)
{
    unsigned int nLevels = sizeof(levelStrings) / sizeof(levelStrings[0]);
    const char* lstr = levelStr.c_str();
    for (unsigned int i = 0; i < nLevels; ++i) {
        if (0 == strcasecmp(lstr, levelStrings[i]))
            return (Logger::Level) i;
    }
    return BAD_LEVEL;
}

/// Create and return the single instance
Logger& opencog::logger()
{
    static Logger instance;
    return instance;
}

// Avoid shared-library crazy-making.
static bool already_loaded = false;
static __attribute__ ((constructor)) void _init(void)
{
    if (already_loaded)
    {
        fprintf(stderr,
            "[FATAL ERROR] Cannot load shared lib more than once!\n");
        exit(1);
    }
    already_loaded = true;
}
