/*
 * Modifications of original are
 *
 * Copyright (C) 2010-2015 OpenCog Foundation
 * All rights reserved.
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

#ifndef _OPENCOG_MT19937AR_H
#define _OPENCOG_MT19937AR_H

#include <opencog/util/RandGen.h>
#include <random>

namespace opencog
{
/** \addtogroup grp_cogutil
 *  @{
 */

class MT19937RandGen : public RandGen
{
public:

    MT19937RandGen(result_type s);

    //! random int between 0 and max rand number.
    int randint();

    //! random float in [0,1]
    float randfloat();

    //! random double in [0,1]
    double randdouble();

    //! random double in [0,1)
    double randdouble_one_excluded();

    //! random int in [0,n)
    int randint(int n);

    //! return -1 or 1 randonly
    int rand_positive_negative();

    //! random boolean
    bool randbool();

    //! random discrete distribution according to some weights
    int rand_discrete(const std::vector<double>&);
};

/**
 * Create and return the single instance. Instance is thread local,
 * so each thread has own copy. The initial seed can be changed
 * using the public method RandGen::seed(unsigned long)
 */
RandGen& randGen();

/** @}*/
} // namespace opencog

#endif // _OPENCOG_MT19937AR_H
