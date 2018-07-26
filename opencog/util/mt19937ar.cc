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


#include "mt19937ar.h"

#include <opencog/util/numeric.h>

//#define  DEBUG_RAND_CALLS
#ifdef DEBUG_RAND_CALLS
#include <opencog/atomspace/Logger.h>
#endif

using namespace opencog;


// PUBLIC METHODS:

MT19937RandGen::MT19937RandGen(result_type s) {
    seed(s);
}


// random int between 0 and max rand number.
int MT19937RandGen::randint() {
    std::uniform_int_distribution<int> dis;
    return dis(*this);
}

// random float in [0,1]
float MT19937RandGen::randfloat() {
    return operator()() / (float)max();
}

// random double in [0,1]
double MT19937RandGen::randdouble() {
	return operator()() / (double)max();
}

//random double in [0,1)
double MT19937RandGen::randdouble_one_excluded() {
    std::uniform_real_distribution<double> dis;
    return dis(*this);
}

//random int in [0,n)
int MT19937RandGen::randint(int n) {
    if ( 0 == n)
        return n;
    else
        return (int)randint() % n;
}

// return -1 or 1 randonly
int MT19937RandGen::rand_positive_negative(){
    return (randint(2) == 0) ? 1 : -1;
}

//random boolean
bool MT19937RandGen::randbool() {
    return randint() % 2 == 0;
}

// random integer values according to a discrete distribution
int MT19937RandGen::rand_discrete(const std::vector<double>& weights)
{
    std::discrete_distribution<int> distribution(weights.begin(), weights.end());
    return distribution(*this);
}

RandGen& opencog::randGen()
{
    static thread_local MT19937RandGen instance(0);
    return instance;
}
