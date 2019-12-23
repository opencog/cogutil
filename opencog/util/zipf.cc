//
// opencog/util/zipf.cc
//
// cut-n-paste of code found here:
// https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
// and then updated with misc fixes.

#include <cmath>
#include <random>

#include "zipf.h"

using namespace opencog;

Zipf::Zipf(double alpha, int n) :
	_n(n)
{
	_norm = 0.0;   // Normalization constant

	// XXX FIXME, this is slow; see
	// https://medium.com/@jasoncrease/zipf-54912d5651cc
	// for something better.
	for (int i=1; i<=n; i++)
		_norm = _norm + pow((double) i, -alpha);

	_norm = 1.0 / _norm;

	_cdf = new double[n+1];
	_cdf[0] = 0.0;
	for (int i=1; i<=n; i++)
		_cdf[i] = _cdf[i-1] + _norm * pow((double) i, -alpha);
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
	int low = 1;
	int high = _n;
	int zipf_draw = 0;
	do
	{
		int mid = floor(0.5 * (low + high));
		if (_cdf[mid] >= z && _cdf[mid-1] < z) {
			zipf_draw = mid;
			break;
		} else if (_cdf[mid] >= z) {
			high = mid-1;
		} else {
			low = mid+1;
		}
	}
	while (low <= high);

	// Assert that zipf_value is between 1 and N
	// assert((zipf_value >=1) && (zipf_value <= n));

	return zipf_draw;
}
