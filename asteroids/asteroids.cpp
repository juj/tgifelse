#include <stdint.h>
#include <math.h>
#include <emscripten/emmalloc.h>
#include <emscripten/html5.h>

extern "C" void logResults(double inputSize, double count, double time);
extern "C" double readInputSize(void);

// Returns 2^n (mod 1e12)
uint64_t Pow2NMod1e12(int n)
{
	if (n < 24)
		return 1ull << n;
	uint64_t res = 16777216ull;
	for(n -= 24; n >= 24; n -= 24)
		res = (res * 16777216ull) % 1000000000000ull;
	return (res * (1ull << n)) % 1000000000000ull;
}

int main()
{
	double inputSize = (int)readInputSize();
	double time0 = emscripten_performance_now();

	int t2 = (int)round(inputSize*2.0);

	// Calculate all divisors of t2, n|t2.
	int *divisors = (int*)malloc(t2*sizeof(int));
	int numDivisors = 0;
	divisors[numDivisors++] = 1;
	for(int i = 2; i < t2; ++i)
		if (t2 % i == 0)
			divisors[numDivisors++] = i;
	divisors[numDivisors++] = t2;

	// Calculate maximal prime power divisors of t2, i.e.
	// p^n | t2, but p^(n+1) does not divide t2.
	int *primePowerDivisors = (int*)malloc(t2*sizeof(int));
	int numPrimePowerDivisors = 0;
	for(int p = 2; p <= t2; ++p)
		if (t2%p == 0)
		{
			int pn;
			for(pn = p; pn*p <= t2 && t2 % (pn*p) == 0;) pn*=p;
			t2 /= pn;
			primePowerDivisors[numPrimePowerDivisors++] = pn;
		}

	// Cache for each divisor[i] a bitfield of which prime power divisors it contains.
	int *divisorPrimePowers = (int*)malloc(numDivisors*sizeof(int));
	for(size_t i = 0; i < numDivisors; ++i)
	{
		uint64_t supports = 0;
		for(size_t j = 0; j < numPrimePowerDivisors; ++j)
			if (divisors[i] % primePowerDivisors[j] == 0)
				supports |= 1ull << j;
		divisorPrimePowers[i] = supports;
	}
	// A bitmask to check when a number contains all the needed prime power divisors.
	const int HaveAllPrimePowers = (1 << numPrimePowerDivisors) - 1;

	// Compute how many ways there are to form a power set from a subset of
	// divisors [idx, ... numDivisors[ that satisfies the orbit period rule.
	uint64_t *partialCounts = (uint64_t*)calloc((numDivisors+1)*(1 << numPrimePowerDivisors), sizeof(uint64_t));
#define KEY(factorIndex, supports) (((supports) * (numDivisors+1)) + (factorIndex))
	for(int idx = numDivisors - 1; idx >= 0; --idx)
		for(int supports = 0; supports <= HaveAllPrimePowers; ++supports)
		{
			uint64_t count = 0;
			for(int nextIdx = idx; nextIdx < numDivisors; ++nextIdx)
			{
				int newSupports = supports | divisorPrimePowers[nextIdx];
				uint64_t c = (newSupports == HaveAllPrimePowers) ? Pow2NMod1e12(numDivisors - 1 - nextIdx) : partialCounts[KEY(nextIdx+1, newSupports)];
				count = (count + c) % 1000000000000ull;
			}
			partialCounts[KEY(idx, supports)] = count;
		}

	// Tally up final answer, form power sets starting from each divisor as the first element.
	uint64_t count = 0;
	for(int i = 0; i < numDivisors; ++i)
		count = (count + partialCounts[KEY(i+1, divisorPrimePowers[i])]) % 1000000000000ull;

	double time1 = emscripten_performance_now();
	logResults(inputSize, (double)count, time1 - time0);
}
