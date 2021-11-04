/*
 * opencog/util/exceptions.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * All Rights Reserved
 *
 * Written by Thiago Maia <thiago@vettatech.com>
 *            Andre Senna <senna@vettalabs.com>
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

#include "exceptions.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <opencog/util/platform.h>
#include <opencog/util/Logger.h>

#define MAX_MSG_LENGTH 2048

using namespace opencog;

/*
 * ----------------------------------------------------------------------
 * StandardException class
 * ----------------------------------------------------------------------
 */
void StandardException::parse_error_message(const char* fmt, va_list ap, bool logError)
{
    char buf[MAX_MSG_LENGTH];

    vsnprintf(buf, sizeof(buf), fmt, ap);

    // If buf contains %s %x %d %u etc. because one of the ap's did,
    // then it becomes a horrid strange crash. So be careful, use %s.
    if (logError) opencog::logger().error("%s", buf);
    set_message(buf);
}

void StandardException::parse_error_message(const char *trace, const char * msg, va_list ap, bool logError)
{
    size_t tlen = 0;
    if (trace) tlen = strlen(trace);

    size_t mlen = strlen(msg);
    char * concatMsg = new char[tlen + mlen + 1];

    strcpy(concatMsg, msg);
    if (trace) {
        // If "trace" is a string that contains %d %s %x %u or any
        // format string, there will be confusing horrors!  Thus,
        // blank out any %'s in "trace".
        char * pcnt = concatMsg + mlen;
        strcpy(pcnt, trace);
        do {
            pcnt = strchr(pcnt, '%');
            if (pcnt) *pcnt = ' ';
        } while (pcnt);
    }

    parse_error_message(concatMsg, ap, logError);

    delete [] concatMsg;
}

StandardException::StandardException() :
    message(nullptr)
{}

// Exceptions must have a copy constructor, as otherwise the
// catcher will not be able to see the message! Ouch!
StandardException::StandardException(const StandardException& ex) :
    message(nullptr)
{
    if (ex.message)
    {
        set_message(ex.message);
    }
}

StandardException& StandardException::operator=(const StandardException& ex)
{
    if (ex.message)
    {
        set_message(ex.message);
    }
    return *this;
}

StandardException::~StandardException() _GLIBCXX_USE_NOEXCEPT
{
    // clear memory
    delete [] message;
}

const char * StandardException::get_message() const
{
    if (message == NULL) {
        return "";
    }
    return message;
}

void StandardException::set_message(const char * msg) const
{
    // clear msg
    delete [] message;

    message = new char[strlen(msg) + 1];
    strcpy(message, msg);
}

/*
 * ----------------------------------------------------------------------
 * RuntimeException class
 * ----------------------------------------------------------------------
 */
RuntimeException::RuntimeException(const char *trace, const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap);
    va_end(ap);
}

RuntimeException::RuntimeException(const char *trace, const char* fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap);
}

RuntimeException::RuntimeException()
{
}

/*
 * ----------------------------------------------------------------------
 * SyntaxException class
 * ----------------------------------------------------------------------
 */
SyntaxException::SyntaxException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap);
    va_end(ap);
}

SyntaxException::SyntaxException(const char * trace, const char * fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap);
}

/*
 * ----------------------------------------------------------------------
 * IOException class
 * ----------------------------------------------------------------------
 */
IOException::IOException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap);
    va_end(ap);
}

IOException::IOException(const char * trace, const char * fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap);
}

/*
 * ----------------------------------------------------------------------
 * ComboException class
 * ----------------------------------------------------------------------
 */
ComboException::ComboException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap);
    va_end(ap);
}

ComboException::ComboException(const char* trace, const char* fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap);
}

/*
 * ----------------------------------------------------------------------
 * IndexErrorException class
 * ----------------------------------------------------------------------
 */
IndexErrorException::IndexErrorException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap);
    va_end(ap);
}

IndexErrorException::IndexErrorException(const char* trace, const char* fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap);
}

/*
 * ----------------------------------------------------------------------
 * InvalidException class
 * ----------------------------------------------------------------------
 */
InvalidParamException::InvalidParamException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap, false);
    va_end(ap);
}

InvalidParamException::InvalidParamException(const char* trace, const char* fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap, false);
}

/*
 * ----------------------------------------------------------------------
 * InconsistenceException class
 * ----------------------------------------------------------------------
 */
InconsistenceException::InconsistenceException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap);
    va_end(ap);
}

InconsistenceException::InconsistenceException(const char* trace, const char* fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap);
}

/*
 * ----------------------------------------------------------------------
 * FatalErrorException class
 * ----------------------------------------------------------------------
 */
FatalErrorException::FatalErrorException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap);
    va_end(ap);
}

FatalErrorException::FatalErrorException(const char* trace, const char* fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap);
}

/*
 * ----------------------------------------------------------------------
 * NetworkException class
 * ----------------------------------------------------------------------
 */
NetworkException::NetworkException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap);
    va_end(ap);
}

NetworkException::NetworkException(const char* trace, const char* fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap);
}

/*
 * ----------------------------------------------------------------------
 * NotFoundException class
 * ----------------------------------------------------------------------
 */
NotFoundException::NotFoundException(const char * trace, const char * fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(trace, fmt, ap, false);
    va_end(ap);
}

NotFoundException::NotFoundException(const char* trace, const char* fmt, va_list ap)
{
    parse_error_message(trace, fmt, ap, false);
}

/*
 * ----------------------------------------------------------------------
 * AssertionException class
 * ----------------------------------------------------------------------
 */
AssertionException::AssertionException(const char* fmt, ...)
{
    va_list  ap;
    va_start(ap, fmt);
    parse_error_message(nullptr, fmt, ap, true);
    va_end(ap);
}

AssertionException::AssertionException(const char* fmt, va_list ap)
{
    parse_error_message(nullptr, fmt, ap, true);
}
