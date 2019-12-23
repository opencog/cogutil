/*
 * opencog/util/zipf.cc
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

#if 0

#include <cmath>
#include <random>

#include "zipf.h"

using namespace opencog;

/// Initialize the Zipfian distribution generator.
/// Parameters:
/// @alpha: double-precision power (equal to +1.0 in the standard zipf)
/// @n: integer, the number of elements in the distribution.
///
/// The implementation here creates an exact CDF (cumulative
/// distribution function) which is both slow, and memory-consuming,
/// if `n` is more than a few thousand, and only a hadnfule of draws
/// are made. So this is a bad solution for large `n`.

Zipf::Zipf(double alpha, int n) :
	_n(n)
{
	// XXX FIXME. For large `n`, one can approximate the CDF.
	// Ideally, one uses an exact CDF for the first few thousand,
	// and then an approximation for the rest.
	_pdf.reserve(n+1);
	_pdf.push_back(0.0);
	for (int i=1; i<=n; i++)
		_pdf.push_back(std::pow((double) i, -alpha));

	_dist = new std::discrete_distribution<int>(_pdf.begin(), _pdf.end());
}

Zipf::~Zipf()
{
	delete _dist;
}

/// Perform one draw, with a Zipf distribution.
/// Return an integer in the range [1,n] includsive.
///
int Zipf::draw()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

#define USE_STD_DIST 1
#if USE_STD_DIST
	return _dist->operator()(gen);

#else
	// Perform a simple (weighted) bisection to invert the CDF.
	// This runs in log(_n) time, which is OK, but not ideal. The
	// std::discrete_distribution() runs a itsy-bitsy tiny bit
	// faster. We keep this code here for some potential future
	// performance improvement for the large-N case, e.g. N=1e6
	// where creating a vector would be sloppy/inefficient. In
	// that case, we can use a discrete vector for the first few
	// hundred, and then use an asymptotic approximation for the
	// rest. In that case, we'd need to revive the code below,
	// its a rough blueprint.
	//
	// Asymptotic expansion is here:
	//   https://medium.com/@jasoncrease/zipf-54912d5651cc
	//
	// FWIW, I experimented with Newton-Rapheson, but that's
	// kind-of tricky, because the CDF is so sharply convex.

	// Pull a uniform random number (0 < u < 1)
	static std::uniform_real_distribution<> uniform(0.0, 1.0);
	double u;
	do
	{
		u = uniform(gen);
	}
	while (u == 0);

	int lo = 1;
	int hi = _n;
	do
	{
		// int mid = (lo + hi) / 2;
		int mid = (7*lo + hi) / 8;
		if (_cdf[mid] >= u and _cdf[mid-1] < u)
			return mid;

		if (_cdf[mid] >= u)
			hi = mid-1;
		else
			lo = mid+1;
	}
	while (lo <= hi);

	return lo;
#endif
}
#endif
