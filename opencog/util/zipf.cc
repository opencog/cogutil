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

Zipf::Zipf(double alpha, int n) :
	_n(n)
{
	// XXX FIXME, this is slow; see
	// https://medium.com/@jasoncrease/zipf-54912d5651cc
	// for something better.
	double norm = 0.0;   // Normalization constant
	_cdf = new double[n+1];
	_cdf[0] = 0.0;
	for (int i=1; i<=n; i++)
	{
		_cdf[i] = _cdf[i-1] + pow((double) i, -alpha);
		norm += cdf[i];
	}

	norm = 1.0 / norm;
	for (int i=1; i<=n; i++)
		_cdf[i] *= norm;
}

int Zipf::draw()
{
	static std::random_device rd;
	std::mt19937 gen(rd());
	static std::uniform_real_distribution<> uniform(0.0, 1.0);

	// Pull a uniform random number (0 < z < 1)
	double z;
	do
	{
		z = uniform(gen);
	}
	while (z == 0);

	// Perform simple bisection to find invert the CDF.
	// It would be faster to perform the Newton-Rapheson here.
	int lo = 1;
	int hi = _n;
	do
	{
		int mid = (lo + hi) / 2;
		if (_cdf[mid] >= z and _cdf[mid-1] < z)
			return mid;

		if (_cdf[mid] >= z)
			hi = mid-1;
		else
			lo = mid+1;
	}
	while (lo <= hi);

	return lo;
}
