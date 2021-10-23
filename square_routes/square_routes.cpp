#include <unordered_map>
#include <algorithm>
#include <stdio.h>
#include <emscripten.h>
#include <emscripten/html5.h>

#define WRAP 1000000000000ull

int N = 8;
int AVERAGE = 64;

uint64_t *fac;

// Caches a factorial table fac[i] = i!
void buildFactorial()
{
	fac = new uint64_t[2*N+1];
	fac[0] = 1;
	for(int i = 1; i < 2*N+1; ++i)
		fac[i] = fac[i-1] * i;
}

// Calculates the total number of routes from upper-left to bottom-right cell of a matrix of size AxB.
uint64_t numRoutesOfSizeAb(int A, int B)
{
	return fac[A+B-2] / (fac[A-1] * fac[B-1]);
}

// Given an input array of ascending sorted numbers 'src', outputs a new array 'dst' that only has the unique
// occurring values in it, and a second array 'duplicateCounts' that counts how many times each input element
// occurred. Returns the length of 'dst'.
int collapseDuplicates(uint32_t *dst, uint32_t *duplicateCounts, uint32_t *src, int srcLen)
{
	uint32_t *dstEnd = dst;
	*dstEnd++ = src[0];
	*duplicateCounts = 1;

	for(int i = 1; i < srcLen; ++i)
		if (dstEnd[-1] != src[i])
			*dstEnd++ = src[i], *++duplicateCounts = 1;
		else
			++*duplicateCounts;
	return dstEnd - dst;
}

int maxBaskets = 0;
uint64_t *kBallsNBasketsCache;

// Returns how many ways k balls (unidentifiable) can be placed into n baskets (identifiable), with
// the restriction that each basket can contain at most 8 balls.
uint64_t kBallsNBaskets(int k, int n)
{
	if (k > 8*n) return 0;

	if (n == 0)
		return 1;
	if (n == 1)
		return k >= 0 && k <= 8 ? 1 : 0;
	if (k == 1)
		return n;
	if (k == 0)
		return 1;

	if (kBallsNBasketsCache[k*(maxBaskets+1) + n])
		return kBallsNBasketsCache[k*(maxBaskets+1) + n];

	uint64_t sum = 0;
	for(int i = 0; i <= 8 && i <= k; ++i)
		sum += kBallsNBaskets(k-i, n-1);
	kBallsNBasketsCache[k*(maxBaskets+1) + n] = sum;
	return sum;
}

uint64_t gcd(uint32_t a, uint32_t b)
{
	while(b)
	{
		uint32_t swap = b;
		b = a % b;
		a = swap;
	}
	return a;
}

uint32_t *factors;
uint32_t *factorCounts;
int numFactors;
uint32_t *weightCeiling;
uint32_t *maxRemainingPower;
uint32_t *remainingGcd;

std::unordered_map<uint64_t, uint64_t> memoizationCache;

// Main routine:
// Calculate the number of solutions to dot product equation (X, K) == m, where
//  - X consists of free variables x_i between [0, weightCeiling[i]].
//  - idx represents the state of search (variables x_0... x_idx-1 have been chosen by caller, recurse into calculating x_idx, ... , x_numFactors)
//  - K is a vector of weight factors from the input matrix.
//  - m is the desired sum for the dot product.
uint64_t numWaysToDotProduct(int idx, int32_t desiredSum)
{
	// The remaining factors will blow the result too big, or not enough remaining power in weights to accumulate to the desired sum -> no way to satisfy sum.
	if (desiredSum < 0 || desiredSum > maxRemainingPower[idx])
		return 0;
	// Last variable of the vector
	if (idx == numFactors - 1)
		return desiredSum % factors[idx] == 0 ? kBallsNBaskets(desiredSum / factors[idx], factorCounts[0]) : 0;
	// Finished the sum already with all subsequent variables equal to zero?
	if (desiredSum == 0)
		return 1;
	// The remaining variables won't be able to solve the dot product due to not having a suitable gcd?
	if (desiredSum % remainingGcd[idx] != 0)
		return 0;

	// See if memoization cache already contains a solution to this subproblem
	uint64_t key = (((uint64_t)idx) << 32) | desiredSum;
	auto iter = memoizationCache.find(key);
	if (iter != memoizationCache.end())
		return iter->second;

	// Calculate how large the variable at this idx must at least have to be, in order to have chances to satisfy the solution?
	int32_t thisMinimumContribution = desiredSum - maxRemainingPower[idx + 1];
	if (thisMinimumContribution < 0) thisMinimumContribution = 0;

	// Fix all possible values for x_i in turn, and recurse to solve x_i+1 onwards.
	uint64_t numWaysToSum = 0;
	for(int i = (thisMinimumContribution + factors[idx] - 1) / factors[idx]; i <= weightCeiling[idx]; ++i)
	{
		uint64_t permutations = kBallsNBaskets(i, factorCounts[idx]) % WRAP;
		uint64_t subproblem = numWaysToDotProduct(idx + 1, desiredSum - i * factors[idx]) % WRAP;
		numWaysToSum = (numWaysToSum + ((permutations * subproblem) % WRAP)) % WRAP;
	}
	// Remember this solution to cache
	memoizationCache[key] = numWaysToSum;
	return numWaysToSum;
}

int main()
{
	N = EM_ASM_INT( return location.search.includes(',') ? parseInt(location.search.substr(1).split(',')[0]) : 8; );
	AVERAGE = EM_ASM_INT( return location.search.includes(',') ? parseInt(location.search.substr(1).split(',')[1]) : 64; );

	double t0 = emscripten_performance_now();
	buildFactorial();

	// Calculate a total count of how many times each cell in matrix N*N can be visited by a path.
	uint32_t *factorMatrix = new uint32_t[N*N];
	uint32_t *f = factorMatrix;
	for(int y = 0; y < N; ++y)
		for(int x = 0; x < N; ++x)
			*f++ = (uint32_t)(numRoutesOfSizeAb(x+1, y+1) * numRoutesOfSizeAb(N-x, N-y));

	// Converting the weight matrix to an ascending weight vector K
	std::sort(factorMatrix, factorMatrix + N*N);

	// Collapse duplicate values in the weight vector.
	factors = new uint32_t[N*N];
	factorCounts = new uint32_t[N*N];
	numFactors = collapseDuplicates(factors, factorCounts, factorMatrix, N*N);

	// Create a memoization cache for the k-balls-in-n-baskets subproblem.
	int largestFactorCount = 0;
	for(int i = 0; i < numFactors; ++i)
		if (factorCounts[i] > largestFactorCount)
			largestFactorCount = factorCounts[i];
	maxBaskets = largestFactorCount;
	kBallsNBasketsCache = new uint64_t[(maxBaskets+1) * (8*maxBaskets+1)];
	memset(kBallsNBasketsCache, 0, sizeof(uint64_t) * (maxBaskets+1) * (8*maxBaskets+1));

	// Calculate the total sum that the dot product must yield.
	uint32_t desiredSum = AVERAGE * (uint32_t)numRoutesOfSizeAb(N, N);
	// Switch from [1, 9] weighted factors to a [0, 8] weighted factors.
	for(int i = 0; i < N*N; ++i)
		desiredSum -= factorMatrix[i];

	// For each input variable x_i, calculate the maximum value that variable can take.
	weightCeiling = new uint32_t[numFactors];
	for(int i = 0; i < numFactors; ++i)
		weightCeiling[i] = 8*factorCounts[i];

	// For partial search state, keep a track of how large the remaining dot product can become.
	// This is used to prune subproblems that cannot possibly lead to a solution.
	maxRemainingPower = new uint32_t[numFactors];
	maxRemainingPower[numFactors-1] = weightCeiling[numFactors-1] * factors[numFactors-1];

	// Also for a partial search state, keep track of the divisibility of the result, to prune
	// search state on target sums that cannot be satisfied due to divisibility rule.
	remainingGcd = new uint32_t[numFactors];
	remainingGcd[numFactors-1] = factors[numFactors-1];
	for(int i = numFactors-2; i >= 0; --i)
	{
		maxRemainingPower[i] = maxRemainingPower[i+1] + weightCeiling[i] * factors[i];
		remainingGcd[i] = gcd(factors[i], remainingGcd[i+1]);
	}

	// Solve:
	uint64_t result = numWaysToDotProduct(0, desiredSum);

	double t1 = emscripten_performance_now();

	EM_ASM({ document.body.innerHTML = `A(${$0}, ${$1}) == ${$2} (mod 1e12). Calculated in ${$3.toFixed(3)} msecs.`}, (double)N, (double)AVERAGE, (double)result, t1-t0);

}
