/*
 * opencog/util/Logger.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * All Rights Reserved
 *
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
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

#ifndef _OPENCOG_LOGGER_H
#define _OPENCOG_LOGGER_H

#include <cstdarg>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <opencog/util/concurrent_queue.h>

namespace opencog
{
/** \addtogroup grp_cogutil
 *  @{
 */

//! logging events
class Logger
{
    void set(const Logger&);
public:

    enum Level { NONE, ERROR, WARN, INFO, DEBUG, FINE, BAD_LEVEL=255 };

    /**
     * Convert from string to enum (ignoring case), and vice-versa.
     */
    static Level get_level_from_string(const std::string&); // Ignoring case
    static const char* get_level_string(const Level);

    // ***********************************************/
    // Constructors/destructors

    /**
     * Messages will be appended to the passed file.
     *
     * @param fileName The log file
     * @param level Only messages with log-level less than or equal to
     *        level will be logged
     * @param timestampEnabled If true, a timestamp will be prefixed to
              every log message
     */
    Logger(const std::string &fileName = "opencog.log",
           Level level = INFO, bool timestampEnabled = true);

    Logger(const Logger&);

    ~Logger();
    Logger& operator=(const Logger& log);

    // ***********************************************/
    // API

    /**
     * Reset the level of messages which will be logged. Every message with
     * log-level lower than or equals to newLevel will be logged.
     */
    void set_level(Level);
    void set_level(const std::string& str) {
        set_level(get_level_from_string(str));
    }

    /**
     * Get the current log level that determines which messages will be
     * logged: Every message with log-level lower than or equals to returned
     * level will be logged.
     */
    Level get_level() const;

    /**
     * Set the level of messages which should be logged with back trace.
     * Every message with log-level lower than or equals to the given argument
     * will have back trace.
     */
    void set_backtrace_level(Level);
    void set_backtrace_level(const std::string& str) {
        set_backtrace_level(get_level_from_string(str));
    }

    /**
     * Get the current back trace log level that determines which messages
     * should be logged with back trace.
     */
    Level get_backtrace_level() const;

    /* filename property */
    void set_filename(const std::string&);
    const std::string& get_filename() const;

    /**
     * Set an optional component string. That string will be inserted
     * between the level and the message, wrapped with brackets, like
     *
     * [2016-01-22 13:59:32:832] [DEBUG] [MyComponent] my log message
     *
     * If the string is empty, as initially, then no string and no
     * brackets are inserted. The above message would look like
     *
     * [2016-01-22 13:59:32:832] [DEBUG] my log message
     */
    void set_component(const std::string&);
    const std::string& get_component() const;

    /**
     * If set, log messages are prefixed with a timestamp.
     */
    void set_timestamp_flag(bool);

    /**
     * If set, log messages are prefixed with a thread id.
     */
    void set_thread_id_flag(bool);

    /**
     * Get the value of the current thread id flag.
     */
    bool get_thread_id_flag() const;

    /**
     * If set, log messages are printed to the stdout.
     */
    void set_print_to_stdout_flag(bool);

    /**
     * Get the value of the current print to stdout flag.
     */
    bool get_print_to_stdout_flag() const;

    /**
     * If set, the logging level is printed as a part
     * of the message,
     */
    void set_print_level_flag(bool);

    /**
     * If set, log messages are printed immediately.
     * (Normally, they are buffered in a different thread, and
     * only get printed when that thread runs. Async logging
     * minimizes the interference to your running program;
     * synchronous logging might slow down your program).
     */
    void set_sync_flag(bool);

    /**
     * Set the main logger to print only
     * error level log on stdout (useful when one is only interested
     * in printing cassert logs)
     */
    void set_print_error_level_stdout();

    /**
     * Log a message into log file (passed in constructor) if and only
     * if passed level is lower than or equal to the current log level
     * of this Logger instance.
     */
    void log(Level level, const std::string &);
    // void error(const std::string &txt);
    // void warn (const std::string &txt);
    // void info (const std::string &txt);
    // void debug(const std::string &txt);
    // void fine (const std::string &txt);

    // Log the backtrace, and only it
    void backtrace();

    /**
     * Log a message (printf style) into log file (passed in constructor)
     * if and only if passed level is lower than or equals to the current
     * log level of this Logger instance.
     *
     * You may use these methods as any printf-style call, eg:
     * fine("Count = %d", count)
     */
    void logva(Level level, const char *, va_list args);
    void log  (Level level, const char *, ...);
    // void error(const char *, ...);
    // void warn (const char *, ...);
    // void info (const char *, ...);
    // void debug(const char *, ...);
    // void fine (const char *, ...);

    class Base
    {
    public:
        Base(const Base& b) : logger(b.logger), lvl(b.lvl) {}
        template<typename T> std::stringstream& operator<<(const T& v)
        {
            ss << v;
            return ss;
        }
        ~Base()
        {
            if (0 < ss.str().length())
                logger.log(lvl, ss.str());
        }
    protected:
        friend class Logger;
        Base(Logger& l, Level v) : logger(l), lvl(v) {}
    private:
        Logger& logger;
        std::stringstream ss;
        Level lvl;
    };

    class Error : public Base
    {
    public:
        void operator()(const std::string &txt) { logger.log(ERROR, txt); }
        void operator()(const char *, ...);
        Base operator()() { return *this; }
    protected:
        friend class Logger;
        Error(Logger& l) : Base(l, ERROR) {}
    };
    Error error;

    class Warn : public Base
    {
    public:
        void operator()(const std::string &txt) { logger.log(WARN, txt); }
        void operator()(const char *, ...);
        Base operator()() { return *this; }
    protected:
        friend class Logger;
        Warn(Logger& l) : Base(l, WARN) {}
    };
    Warn warn;

    class Info : public Base
    {
    public:
        void operator()(const std::string &txt) { logger.log(INFO, txt); }
        void operator()(const char *, ...);
        Base operator()() { return *this; }
    protected:
        friend class Logger;
        Info(Logger& l) : Base(l, INFO) {}
    };
    Info info;

    class Debug : public Base
    {
    public:
        void operator()(const std::string &txt) { logger.log(DEBUG, txt); }
        void operator()(const char *, ...);
        Base operator()() { return *this; }
    protected:
        friend class Logger;
        Debug(Logger& l) : Base(l, DEBUG) {}
    };
    Debug debug;

    class Fine : public Base
    {
    public:
        void operator()(const std::string &txt) { logger.log(FINE, txt); }
        void operator()(const char *, ...);
        Base operator()() { return *this; }
    protected:
        friend class Logger;
        Fine(Logger& l) : Base(l, FINE) {}
    };

    Fine fine;

public:
    /**
     * Methods to check if a given log level is enabled. This is useful for
     * avoiding unnecessary code for logger. For example:
     * if (isDebugEnabled())  debug(...);
     */
    bool is_enabled(Level level) const { return level <= currentLevel; }
    bool is_error_enabled() const { return ERROR <= currentLevel; }
    bool is_warn_enabled() const { return WARN <= currentLevel; }
    bool is_info_enabled() const { return INFO <= currentLevel; }
    bool is_debug_enabled() const { return DEBUG <= currentLevel; }
    bool is_fine_enabled() const { return FINE <= currentLevel; }

    /**
     * Block until all messages have been written out.
     */
    void flush();

private:

    std::string component;
    Level currentLevel;
    Level backTraceLevel;
    bool timestampEnabled;
    bool threadIdEnabled;
    bool logEnabled;
    bool printToStdout;
    bool printLevel;
    bool syncEnabled;

    /**
     * Enable logging messages.
     */
    void enable();

    /**
     * Disable logging messages.
     */
    void disable();

    class LogWriter
    {
        /* One writer per file */
        std::string fileName;
        FILE *logfile;
        bool writingLoopActive;

        /** One single thread does all writing of log messages */
        std::thread writer_thread;
        std::mutex the_mutex;

        /** Queue for log messages */
        concurrent_queue< std::string* > msg_queue;
        bool pending_write;

        void start_write_loop();
        void stop_write_loop();
        void writing_loop();
        void write_msg(const std::string&);

    public:
        LogWriter(void);
        ~LogWriter();

        void setFileName(const std::string&);

        const std::string& getFileName(void) const
            { return fileName; }

        void qmsg(const std::string& str)
            { msg_queue.push(new std::string(str)); }

        size_t size(void)
            { return msg_queue.size(); }

        void flush();
    };

    LogWriter* _log_writer;

    static std::mutex _loggers_mtx;
    static std::map<std::string, LogWriter*> _loggers;

}; // class

// A singleton instance is enough for most users.
Logger& logger();

// Macros that avoid evaluating the stream if the log-level is disabled
#define LAZY_LOG_ERROR if(logger().is_error_enabled()) logger().error()
#define LAZY_LOG_WARN if(logger().is_warn_enabled()) logger().warn()
#define LAZY_LOG_INFO if(logger().is_info_enabled()) logger().info()
#define LAZY_LOG_DEBUG if(logger().is_debug_enabled()) logger().debug()
#define LAZY_LOG_FINE if(logger().is_fine_enabled()) logger().fine()

/** @}*/
}  // namespace opencog

#endif // _OPENCOG_LOGGER_H
