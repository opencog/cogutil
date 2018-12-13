/*
 * opencog/util/RandGen.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * All Rights Reserved
 *
 * Written by Welter Silva <welter@vettalabs.com>
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

#ifndef _OPENCOG_RAND_GEN_H
#define _OPENCOG_RAND_GEN_H

#include <set>
#include <vector>
#include <opencog/util/exceptions.h>
#include <random>

namespace opencog
{
/** \addtogroup grp_cogutil
 *  @{
 */

//! interface for random generators

// RandGen inherits std::mt19937. This may seem like a hack because
// there is no point having that abstract class inherit a concrete
// random generator. But it's clear we don't need that home made
// random generator structure anymore so it's a step toward getting
// rid of it.
//
// The advantage is that the random generator returned by the
// singleton randGen() can be now used in the STL distributions.
class RandGen : public std::mt19937
{

public:

    virtual ~RandGen() {}

    //! random int between 0 and max rand number.
    virtual int randint() = 0;

    //! random float in [0,1]
    virtual float randfloat() = 0;

    //! random double in [0,1]
    virtual double randdouble() = 0;

    //! random double in [0,1)
    virtual double randdouble_one_excluded() = 0;

    //! random int in [0,n)
    virtual int randint(int n) = 0;

    //! return -1 or 1 randonly
    virtual int rand_positive_negative() = 0;

    //! random boolean
    virtual bool randbool() = 0;

    //! random discrete base on weights
    virtual int rand_discrete(const std::vector<double>&) = 0;
};

/** @}*/
} // ~namespace opencog

#endif // _OPENCOG_RAND_GEN_H
