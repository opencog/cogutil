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

// Overall idea taken from here:
//   https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
// with a theoretical overview from here:
//   https://medium.com/@jasoncrease/zipf-54912d5651cc
// and then updated so that its vaguely respectable code.

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
	double norm = 0.0;   // Normalization constant
	_cdf = new double[n+1];
	_cdf[0] = 0.0;
	for (int i=1; i<=n; i++)
		_cdf[i] = _cdf[i-1] + std::pow((double) i, -alpha);

	norm = 1.0 / _cdf[n];
	for (int i=1; i<=n; i++)
		_cdf[i] *= norm;
}

/// Perform one draw, with a ZZipf distribution.
/// Return an integer in the range [1,n] includsive.
///
int Zipf::draw()
{
	static std::random_device rd;
	std::mt19937 gen(rd());
	static std::uniform_real_distribution<> uniform(0.0, 1.0);

	// Pull a uniform random number (0 < u < 1)
	double u;
	do
	{
		u = uniform(gen);
	}
	while (u == 0);

#define BISECT 1
#if BISECT
	// Perform simple bisection to find invert the CDF.
	// This runs in log(_n) time, whichy is OK, but perhaps
	// not ideal.
	int lo = 1;
	int hi = _n;
	do
	{
		int mid = (lo + hi) / 2;
		if (_cdf[mid] >= u and _cdf[mid-1] < u)
			return mid;

		if (_cdf[mid] >= u)
			hi = mid-1;
		else
			lo = mid+1;
	}
	while (lo <= hi);

	return lo;
#else
	// Perform Newton-Rapheson, for speed.
	int lo = 1;
	int hi = _n;

	double flo = _cdf[lo];
	double fhi = _cdf[hi];
	do
	{
printf("duude flohi= %g %g\n", flo, fhi);
		double slp = (hi - lo) / (fhi - flo);
		int mid = lo - (flo-u) * slp;
printf("duuude iter u=%g lo=%d hi=%d mid=%d slp=%g\n", u, lo, hi, mid, slp);
fflush (stdout);
		if (_cdf[mid] >= u and _cdf[mid-1] < u)
			return mid;

		if (_cdf[mid] >= u)
		{
			hi = mid-1;
			fhi = _cdf[hi];
		}
		else
		{
			lo = mid+1;
			flo = _cdf[lo];
		}
	}
	while (lo <= hi);

	return lo;
#endif
}
