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
 * The solve purpose of this file is to contain a static empty
 * std::string whenever it is needed (which turns out to be
 * often). Beside that it useful so that oc_to_string (pretty printer
 * for OpenCog C++ objects) can be called with an empty initial
 * indentation within gdb, since the latter isn't able to allocate an
 * empty string.
 */

#include <string>

#ifndef _OPENCOG_EMPTY_STRING_H_
#define _OPENCOG_EMPTY_STRING_H_

namespace opencog {

// Empty static string. This is mostly to make oc_to_string and poc
// (see
// https://wiki.opencog.org/w/Development_standards#Pretty_Print_OpenCog_Objects)
// easier to define.
const static std::string empty_string = "";

// Default indentation for oc_to_string when there is some.
const static std::string oc_to_string_indent = "  ";

} // ~namespace opencog

#endif
