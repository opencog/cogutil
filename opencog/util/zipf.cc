//
// opencog/util/zipf.cc
//
// cut-n-paste of code found here:
// https://stackoverflow.com/questions/9983239/how-to-generate-zipf-distributed-numbers-efficiently
// and then updated with misc fixes.

#include <cmath>
#include <random>

int zipf(double alpha, int n)
{
	static bool first = true;     // Static first time flag
	static double c = 0;          // Normalization constant
	static double *sum_probs;     // Pre-calculated sum of probabilities
	double z;                     // Uniform random number (0 < z < 1)

	static std::random_device rd;
	std::mt19937 gen(rd());
	static std::uniform_real_distribution<> uniform(0.0, 1.0);

	// Compute normalization constant on first call only
	if (first)
	{
		for (int i=1; i<=n; i++)
			c = c + (1.0 / pow((double) i, alpha));

		c = 1.0 / c;

		sum_probs = (double *) malloc((n+1)*sizeof(*sum_probs));
		sum_probs[0] = 0;
		for (int i=1; i<=n; i++)
			sum_probs[i] = sum_probs[i-1] + c / pow((double) i, alpha);

		first = false;
	}

	// Pull a uniform random number (0 < z < 1)
	do
	{
		z = uniform(gen);
	}
	while (z == 0);

	int low = 1;
	int high = n;
	int zipf_draw = 0;
	do
	{
		int mid = floor(0.5 * (low + high));
		if (sum_probs[mid] >= z && sum_probs[mid-1] < z) {
			zipf_draw = mid;
			break;
		} else if (sum_probs[mid] >= z) {
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
