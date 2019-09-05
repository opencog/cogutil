/*
 * opencog/util/numeric.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * All Rights Reserved
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

#ifndef _OPENCOG_NUMERIC_H
#define _OPENCOG_NUMERIC_H

#include <algorithm> // for std::max
#include <cmath>
#include <climits>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <vector>

#include <boost/range/numeric.hpp>

#include <opencog/util/iostreamContainer.h>
#include <opencog/util/oc_assert.h>

/** \addtogroup grp_cogutil
 *  @{
 */

namespace opencog
{

// Maximum acceptable difference when comparing probabilities.
#define PROB_EPSILON 1e-127

// Maximum acceptable difference when comparing distances.
#define DISTANCE_EPSILON 1e-32

//! absolute_value_order
//!   codes the following order, for T == int, -1,1,-2,2,-3,3,...
template<typename T>
struct absolute_value_order
{
    bool operator()(T a,T b) const {
        return (a == -b) ? a < b : std::abs(a) < std::abs(b);
    }
};

/// Return the index of the first non-zero bit in the integer value,
/// minus one.
inline unsigned int integer_log2(size_t v)
{
#ifdef __GNUC__
    // On x86_64, this uses the BSR (Bit Scan Reverse) insn or
    // the LZCNT (Leading Zero Count) insn.
    // This should work on all arches; its part of GIL/gimple.
    if (0 == v) return 0;
    return (8*sizeof(size_t) - 1) - __builtin_clzl(v);
#else
    static const int MultiplyDeBruijnBitPosition[32] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };

    v |= v >> 1; // first round down to power of 2
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v = (v >> 1) + 1;
    return MultiplyDeBruijnBitPosition[static_cast<uint32_t>(v * 0x077CB531UL) >> 27];
#endif
}

//! return p the smallest power of 2 so that p >= x
/// So for instance:
///   - next_power_of_two(1) = 1
///   - next_power_of_two(2) = 2
///   - next_power_of_two(3) = 4
inline size_t next_power_of_two(size_t x)
{
    OC_ASSERT(x > 0);
#ifdef __GNUC__
    if (1==x) return 1;  // because __builtin_clzl(0) is -MAX_INT
    return 1UL << (8*sizeof(size_t) - __builtin_clzl(x-1));
#else
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
#endif
}

/// Return the number of bits needed to hold the value, aligned to
/// the next power of two. This is used by moses, to pack int values
/// into bits.  It is asking for a power-of-two alignment, presumably
/// with the idea that the compiler generates faster code!? However,
/// the unit tests don't seem to actually require this; it works
/// just fine, (and not any slower) if alignment is discarded.
/// That is, the moses unit tests run equally fast, and pass, when
/// #define ALIGNED_NOT_ACTUALLY_REQUIRED 1 is set.
///
/// Examples of alignment:
///    - nbits_to_pack(2) = 1,
///    - nbits_to_pack(3) = 2,
///    - nbits_to_pack(50) = 8  (not 6)
///    - nbits_to_pack(257) = 16  (not 9)
///
inline unsigned int nbits_to_pack(size_t multy)
{
    OC_ASSERT(multy > 0);
// #define ALIGNED_NOT_ACTUALLY_REQUIRED 1
#ifdef ALIGNED_NOT_ACTUALLY_REQUIRED
    return integer_log2(multy -1) + 1;
#else
    return next_power_of_two(integer_log2(multy -1) + 1);
#endif
}

//! returns true iff x >= min and x <= max
template<typename FloatT> bool is_between(FloatT x, FloatT min_, FloatT max_)
{
    return x >= min_ and x <= max_;
}

/// Compare floats with ULPS, because they are lexicographically
/// ordered. For technical explanation, see
/// http://www.cygnus-software.com/papers/comparingfloats/Comparing%20floating%20point%20numbers.htm
/// and also it's new, improved variant:
/// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
///
static inline bool is_approx_eq_ulp(double x, double y, int64_t max_ulps)
{
	if ((x < 0) != (y < 0))
	{
		return x == y; // in case x==0 and y==-0
	}

	// Some compilers may need the __may_alias__ attribute to
	// prevent a compiler warning.
	//     warning: dereferencing type-punned pointer will break
	//     strict-aliasing rules
	// int64_t* __attribute__((__may_alias__)) xbits = ...
	int64_t* xbits = reinterpret_cast<int64_t*>(&x);
	int64_t* ybits = reinterpret_cast<int64_t*>(&y);

	// The dereferencing is below is explicitly called out as
	// "undefined behavior" by the C++ spec.  However, the spec
	// also tells us how to do with correcly, when C++20 becomes
	// available:
	// std::abs(std::bit_cast<std::int64_t>(x) - std::bit_cast<std::int64_t>(y))

	static_assert(sizeof(int64_t) == sizeof(double), "Unexpected sizeof(double)");

	int64_t ulps = std::abs(*xbits - *ybits);
	return max_ulps > ulps;
}

//! returns true iff abs(x - y) <= epsilon
template<typename FloatT> bool is_within(FloatT x, FloatT y, FloatT epsilon)
{
    return std::abs(x - y) <= epsilon;
}

//! compare 2 FloatT with precision epsilon,
/// note that, unlike isWithin, the precision adapts with the scale of x and y
template<typename FloatT> bool is_approx_eq(FloatT x, FloatT y, FloatT epsilon)
{
    FloatT diff = std::fabs(x - y);
    if (diff < epsilon) return true;

    FloatT amp = std::fabs(x + y);
    return diff <= epsilon * amp;
}

// TODO: replace the following by C++17 std::clamp
/**
 * Return x clamped to [l, u], that is it returns max(l, min(u, x))
 */
template<typename Float>
Float clamp(Float x, Float l, Float u)
{
    return std::max(l, std::min(u, x));
}

//! useful for entropy
template<typename FloatT> FloatT weighted_information(FloatT p)
{
    return p > PROB_EPSILON? -p * std::log2(p) : 0;
}

//! compute the binary entropy of probability p
template<typename FloatT> FloatT binary_entropy(FloatT p)
{
    OC_ASSERT(p >= 0 and p <= 1,
              "binaryEntropy: probability %f is not between 0 and 1", p);
    return weighted_information(p) + weighted_information(1.0 - p);
}

/**
 * Compute entropy of an n-ary probability distribution, with the
 * probabilities pointed at by iterators (from, to[.
 * Specifically it computes
 *
 * - Sum_i p_i log_2(p_i)
 *
 * where the p_i are values pointed by (from, to[,
 * It is assumed that Sum_i p_i == 1.0
 * That is, std::accumulate(from, to, 0) == 1.0
 */
template<typename It> double entropy(It from, It to)
{
    double res = 0;
    for(; from != to; ++from)
        res += weighted_information(*from);
    return res;
}

//! helper
template<typename C>
double entropy(const C& c)
{
    return entropy(c.begin(), c.end());
}

//! compute the smallest divisor of n
template<typename IntT> IntT smallest_divisor(IntT n)
{
    OC_ASSERT(n > 0, "smallest_divisor: n must be superior than 0");
    if(n<3)
        return n;
    else {
        bool found_divisor = false;
        IntT i = 2;
        for(; i*i <= n and !found_divisor; i++) {
            found_divisor = n%i==0;
        }
        if(found_divisor)
            return i-1;
        else return n;
    }
}

//! calculate the square of x
template<typename T> T sq(T x) { return x*x; }

//! check if x isn't too high and return 2^x
template<typename OutInt> OutInt pow2(unsigned int x)
{
    OC_ASSERT(8*sizeof(OutInt) - (std::numeric_limits<OutInt>::is_signed?1:0) > x,
              "pow2: Amount to shift is out of range.");
    return static_cast<OutInt>(1) << x;
}
inline unsigned int pow2(unsigned int x) { return pow2<unsigned int>(x); }

//! Generalized mean http://en.wikipedia.org/wiki/Generalized_mean
template<typename It, typename Float>
Float generalized_mean(It from, It to, Float p = 1.0)
{
    Float pow_sum =
        std::accumulate(from, to, 0.0,
                        [&](Float l, Float r) { return l + pow(r, p); });
    return pow(pow_sum / std::distance(from, to), 1.0 / p);
}
template<typename C, typename Float>
Float generalized_mean(const C& c, Float p = 1.0)
{
    return generalized_mean(c.begin(), c.end(), p);
}

/// Compute the distance between two vectors, using the p-norm.  For
/// p=2, this is the usual Eucliden distance, and for p=1, this is the
/// Manhattan distance, and for p=0 or negative, this is the maximum
/// difference for one element.
template<typename Vec, typename Float>
Float p_norm_distance(const Vec& a, const Vec& b, Float p=1.0)
{
    OC_ASSERT (a.size() == b.size(),
               "Cannot compare unequal-sized vectors!  %d %d\n",
               a.size(), b.size());

    typename Vec::const_iterator ia = a.begin(), ib = b.begin();

    Float sum = 0.0;
    // Special case Manhattan distance.
    if (1.0 == p) {
        for (; ia != a.end(); ++ia, ++ib)
            sum += fabs (*ia - *ib);
        return sum;
    }
    // Special case Euclidean distance.
    if (2.0 == p) {
        for (; ia != a.end(); ++ia, ++ib)
            sum += sq (*ia - *ib);
        return sqrt(sum);
    }
    // Special case max difference
    if (0.0 >= p) {
        for (; ia != a.end(); ++ia, ++ib) {
            Float diff = fabs (*ia - *ib);
            if (sum < diff) sum = diff;
        }
        return sum;
    }

    // General case.
    for (; ia != a.end(); ++ia, ++ib) {
        Float diff = fabs (*ia - *ib);
        if (0.0 < diff)
            sum += pow(diff, p);
    }
    return pow(sum, 1.0/p);
}

/**
 * Return the Tanimoto distance, a continuous extension of the Jaccard
 * distance, between 2 vector.
 *
 * See http://en.wikipedia.org/wiki/Jaccard_index
 *
 * More specifically we use
 *
 * 1 - f(a,b)
 *
 * where
 *
 * f(a,b) = (sum_i a_i * b_i) / (sum_i a_i^2 + sum_i b_i^2 - sum_i a_i * b_i)
 *
 * If a and b are binary vectors then this corresponds to the Jaccard
 * distance. Otherwise, anything goes, it's not even a true metric
 * anymore, but it could be useful anyway.
 *
 * Warning: if a and b have negative elements then the result could be
 * negative. In that case you might prefer to use the angular
 * distance.
 *
 * Indeed the Tanimoto distance is considered as a generalization of
 * the Jaccard distance over multisets (where the weights are the
 * number of occurences, therefore never negative). See
 * http://en.wikipedia.org/wiki/Talk%3AJaccard_index
 */
template<typename Vec, typename Float>
Float tanimoto_distance(const Vec& a, const Vec& b)
{
    OC_ASSERT (a.size() == b.size(),
               "Cannot compare unequal-sized vectors!  %d %d\n",
               a.size(), b.size());

    Float ab = boost::inner_product(a, b, Float(0)),
        aa = boost::inner_product(a, a, Float(0)),
        bb = boost::inner_product(b, b, Float(0)),
        numerator = aa + bb - ab;

    if (numerator >= Float(DISTANCE_EPSILON))
        return 1 - (ab / numerator);
    else
        return 0;
}

/**
 * Return the angular distance between 2 vectors.
 *
 * See http://en.wikipedia.org/wiki/Cosine_similarity (Also, according
 * to Linas this is also equivalent to Fubini-Study metric, Fisher
 * information metric, and Kullbeck-Liebler divergence but they look
 * so differently so that anyone can hardly see it).
 *
 * Specifically it computes
 *
 * f(a,b) = alpha*cos^{-1}((sum_i a_i * b_i) /
 *                         (sqrt(sum_i a_i^2) * sqrt(sum_i b_i^2))) / pi
 *
 * with alpha = 1 where vector coefficients may be positive or negative
 * or   alpha = 2 where the vector coefficients are always positive.
 *
 * If pos_n_neg == true then alpha = 1, otherwise alpha = 2.
 */
template<typename Vec, typename Float>
Float angular_distance(const Vec& a, const Vec& b, bool pos_n_neg = true)
{
    OC_ASSERT (a.size() == b.size(),
               "Cannot compare unequal-sized vectors!  %d %d\n",
               a.size(), b.size());

    // XXX FIXME writing out the explicit loop will almost
    // surely be faster than calling boost. Why? Because a single
    // loop allows the compiler to insert instructions into the
    // pipeline bubbles; whereas three different loops will be more
    // than three times slower!
    Float ab = boost::inner_product(a, b, Float(0)),
        aa = boost::inner_product(a, a, Float(0)),
        bb = boost::inner_product(b, b, Float(0)),
        numerator = sqrt(aa * bb);

    if (numerator >= Float(DISTANCE_EPSILON)) {
        // in case of rounding error
        Float r = clamp(ab / numerator, Float(-1), Float(1));
        return (pos_n_neg ? 1 : 2) * acos(r) / M_PI;
    }
    else
        return 0;
}

// Avoid spewing garbage into the namespace!
#undef PROB_EPSILON
#undef DISTANCE_EPSILON

} // ~namespace opencog
/** @}*/

#endif // _OPENCOG_NUMERIC_H
