/*
 * opencog/util/zipf.h
 *
 * Copyright (C) 2019 Linas Vepstas
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

#ifndef _OPENCOG_ZIPF_H
#define _OPENCOG_ZIPF_H

#include <algorithm>
#include <cmath>
#include <random>

namespace opencog {
/** \addtogroup grp_cogutil
 *  @{
 */

/** Zipf-like random distribution.
 *
 * Implementation taken from drobilla's May 24, 2017 answer to
 * https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
 *
 * That code is referenced with this:
 * "Rejection-inversion to generate variates from monotone discrete
 * distributions", Wolfgang Hörmann and Gerhard Derflinger
 * ACM TOMACS 6.3 (1996): 169-184
 *
 * This code works best for large-N distributions; for small-N
 * use the SmallZipf class, further below.
 *
 * Example usage:
 *
 *    std::random_device rd;
 *    std::mt19937 gen(rd());
 *    Zipf<> zipf(300);
 *
 *    for (int i = 0; i < 100; i++)
 *        printf("draw %d %d\n", i, zipf(gen));
 */

template<class IntType = unsigned long, class RealType = double>
class Zipf
{
	public:
		static_assert(std::numeric_limits<IntType>::is_integer, "");
		static_assert(!std::numeric_limits<RealType>::is_integer, "");

		Zipf(const IntType n=std::numeric_limits<IntType>::max(),
		     const RealType q=1.0)
        : n(n)
        , q(q)
        , H_x1(H(1.5) - 1.0)
        , H_n(H(n + 0.5))
        , dist(H_x1, H_n)
		{}

		IntType operator()(std::mt19937& rng)
		{
			while (true) {
				const RealType u = dist(rng);
				const RealType x = H_inv(u);
				const IntType  k = clamp<IntType>(std::round(x), 1, n);
				if (u >= H(k + 0.5) - h(k)) {
					return k;
				}
			}
		}

	private:
		static constexpr RealType epsilon = 1e-8;
		IntType                                  n;     ///< Number of elements
		RealType                                 q;     ///< Exponent
		RealType                                 H_x1;  ///< H(x_1)
		RealType                                 H_n;   ///< H(n)
		std::uniform_real_distribution<RealType> dist;  ///< [H(x_1), H(n)]

		/** Clamp x to [min, max]. */
		template<typename T>
		static constexpr T clamp(const T x, const T min, const T max)
		{
			return std::max(min, std::min(max, x));
		}

		/** exp(x) - 1 / x */
		static double
		expxm1bx(const double x)
		{
			return (std::abs(x) > epsilon)
			    ? std::expm1(x) / x
			    : (1.0 + x/2.0 * (1.0 + x/3.0 * (1.0 + x/4.0)));
		}

		/** H(x) = log(x) if q == 1, (x^(1-q) - 1)/(1 - q) otherwise.
		 * H(x) is an integral of h(x).
		 *
		 * Note the numerator is one less than in the paper order to
		 * work with all positive q.
		 */
		const RealType H(const RealType x)
		{
			const RealType log_x = std::log(x);
			return expxm1bx((1.0 - q) * log_x) * log_x;
		}

		/** log(1 + x) / x */
		static RealType
		log1pxbx(const RealType x)
		{
			return (std::abs(x) > epsilon)
			    ? std::log1p(x) / x
			    : 1.0 - x * ((1/2.0) - x * ((1/3.0) - x * (1/4.0)));
		}

		/** The inverse function of H(x) */
		const RealType H_inv(const RealType x)
		{
			const RealType t = std::max(-1.0, x * (1.0 - q));
			return std::exp(log1pxbx(t) * x);
		}

		/** That hat function h(x) = 1 / (x ^ q) */
		const RealType h(const RealType x)
		{
			return std::exp(-q * std::log(x));
		}
};

/**
 * Same API as above, but about 2.3x faster for N=300.
 * I'm not sure where the cross-over is.
 */
template<class IntType = unsigned long, class RealType = double>
class ZipfSmall
{
	private:
		std::vector<RealType> _pdf;
		std::discrete_distribution<IntType>* _dist;
	public:
		ZipfSmall(const IntType n,
		          const RealType q=1.0)
		{
			_pdf.reserve(n+1);
			_pdf.push_back(0.0);
			for (IntType i=1; i<=n; i++)
				_pdf.push_back(std::pow((double) i, -q));

			_dist = new std::discrete_distribution<IntType>(_pdf.begin(), _pdf.end());
		}

		~ZipfSmall()
		{
			delete _dist;
		}

		IntType operator()(std::mt19937& rng)
		{
			return _dist->operator()(rng);
		}
};

/** @}*/
} // ~namespace opencog

#endif // _OPENCOG_ZIPF_H
