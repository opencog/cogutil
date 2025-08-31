/*
 * empty_string.h
 *
 * Copyright (C) 2018 OpenCog Foundation
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

/**
 * The sole purpose of this file is to provide a static empty
 * std::string, to be used as a default function initializer.
 * This allows gdb to call those functions, as otherwise gdb is
 * not able to allocate an empty string.
 *
 * Example usage:
 *   int foo(std::string = "");            // BAD gdb cannot call this!
 *   int foo(std::string = empty_string);  // GOOD gdb can call this.
 */

#include <string>

#ifndef _OPENCOG_EMPTY_STRING_H_
#define _OPENCOG_EMPTY_STRING_H_

namespace opencog {

// Empty static string. This is mostly to make oc_to_string and poc
// (see
// https://wiki.opencog.org/w/Development_standards#Pretty_Print_OpenCog_Objects)
// easier to define.
static inline const std::string empty_string = "";

// Default indentation for oc_to_string when there is some.
static inline const std::string oc_to_string_indent = "  ";

} // ~namespace opencog

#endif
