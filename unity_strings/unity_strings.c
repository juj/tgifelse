#include <stdint.h>
#include <emscripten.h>
#include <emscripten/html5.h>

/* Denote by <N> set of all strings of length N that do not contain substring "UNITY" in it.
   Denote by [N] set of all strings of length N that have exactly one substring "UNITY" in it.
   Denote by |[N]| the count of different possible strings [N] (and |<N>| the number of different
   possible strings <N>)

   Observations:
    1. If we have a string of length N with no substring "UNITY" in it,
       if we insert "UNITY" somewhere in the middle of the string at position K, to get a string
       <K>UNITY<N-K>, that string will have exactly one occurrence of "UNITY" in it.
       That is, inserting "UNITY" in to a random string cannot cause two "UNITY" substrings to
       appear. (this would not be the case e.g. for string "UNITU", since prepending "UNITU"
       to a string "NITU" for "UNITUNITU" would create two occurrences of the substring - but
       string "UNITY" does not have that behavior)

    2. Hence to calculate |[N]|, we can instead enumerate all possible strings <N-5> that do not
       have UNITY in them, and then for each such string, insert UNITY into them in all possible
       subpositions. That is,

       [N] = <0>UNITY<N-5> u <1>UNITY<N-6> u <2>UNITY<N-7> u ... u <N-5>UNITY<0>

      where 'u' stands for union, and hence

       |[N]| = |<0>|*|<N-5>| + |<1>|*|<N-6>| + |<2>|*|<N-7>| + ... + |<N-5>|*|<0>|     (*1)

      so this means we need to calculate |<0>|, |<1>|, ..., |<N-5>| to find the final answer.

    To solve <K>, we can use recursion. Denote by <K-1,!NITY> the set of all strings of length
    K-1 that do not contain UNITY in them, but also do not start with substring "NITY". Then,

      <K> = U<K-1,!NITY> u N<K-1> u I<K-1> u T<K-1> u Y<K-1>

    or slightly more succinctly written:

      <K>   = U<K-1,!NITY>  u {N,I,T,Y}<K-1>
      |<K>| = |<K-1,!NITY>| +     4 * |<K-1>|                                          (*2)

    We continue the recurrence to break down <K-1,!NITY> to smaller parts:

       <K,!NITY>  = U<K-1,!NITY>  u  N<K-1,!ITY>  u I<K-1> u T<K-1> u Y<K-1>
       <K,!NITY>  = U<K-1,!NITY>  u {I,T,Y}<K-1>  u N<K-1,!ITY>
      |<K,!NITY>| = |<K-1,!NITY>| +   3 * |<K-1>| + |<K-1,!ITY>|                       (*3)

    and then break down <K-1,!ITY>:

       <K,!ITY>  = U<K-1,!NITY>  u       N<K-1>  u I<K-1,!TY> u T<K-1> u Y<K-1>
       <K,!ITY>  = U<K-1,!NITY>  u {N,T,Y}<K-1>  u I<K-1,!TY>
      |<K,!ITY>| = |<K-1,!NITY>| +   3 * |<K-1>| + |<K-1,!TY>|                         (*4)

    and <K-1,!TY>:

       <K,!TY>  =   U<K-1,!NITY>  u N<K-1>        u I<K-1> u T<K-1,!Y> u Y<K-1>
       <K,!TY>  =   U<K-1,!NITY>  u {N,I,Y}<K-1>  u T<K-1,!Y>
      |<K,!TY>| =   |<K-1,!NITY>| +   3 * |<K-1>| + |<K-1,!Y>|                         (*5)

    and finally <K-1,!Y>:

       <K,!Y>  = U<K-1,!NITY> u       N<K-1> u I<K-1> u T<K-1,!Y>
       <K,!Y>  = U<K-1,!NITY> u {N,I,T}<K-1>
      |<K,!Y>| = |<K-1,!NITY>| +  3 * |<K-1>|                                          (*6)

    Define base cases for strings of length 0 and 1, and then use the starred equations
    in a bottom up manner: */

// Largest input size
#define MAX_N 10000

// Reduce back to 1e9 range
#define MOD10(x) ((x) % 1000000000ull)

// y[i]:     |<i,!Y>|,    or the number of strings of length i that do not start with "Y", and do not contain substring "UNITY" in them.
// ty[i]:    |<i,!TY>|,   The number of strings of length i that do not start with "TY", and do not contain substring "UNITY" in them.
// ity[i]:   |<i,!ITY>|,  The number of strings of length i that do not start with "ITY", and do not contain substring "UNITY" in them.
// nity[i]:  |<i,!NITY>|, The number of strings of length i that do not start with "NITY", and do not contain substring "UNITY" in them.
// unity[i]: |<i>|,       The number of strings of length i that do not contain substring "UNITY" in them.
uint64_t y[MAX_N]     = { 1, 4 };
uint64_t ty[MAX_N]    = { 1, 5 };
uint64_t ity[MAX_N]   = { 1, 5 };
uint64_t nity[MAX_N]  = { 1, 5 };
uint64_t unity[MAX_N] = { 1, 5 };

int main()
{
	int n = EM_ASM_INT(return parseInt(location.search.substr(1) || '1000'));
	if (n > MAX_N) n = MAX_N;

	double t = emscripten_performance_now();
	n -= 5; // Remove characters for "UNITY" from the count.
	for(int i = 2; i <= n; ++i)
	{
		y[i]     = MOD10(nity[i-1] + 3*unity[i-1]);             // (*6)
		ty[i]    = MOD10(nity[i-1] + 3*unity[i-1] + y[i-1]);    // (*5)
		ity[i]   = MOD10(nity[i-1] + 3*unity[i-1] + ty[i-1]);   // (*4)
		nity[i]  = MOD10(nity[i-1] + 3*unity[i-1] + ity[i-1]);  // (*3)
		unity[i] = MOD10(nity[i-1] + 4*unity[i-1]);             // (*2)
	}

	// Micro-optimization: final accumulation is symmetric around n/2.
	uint64_t count = 0;                                        // (*1)
	for(int i = 0; i < (n+1)/2; ++i)                           // (*1)
		count = MOD10(count + unity[i] * unity[n-i]);           // (*1)
	count = MOD10(count*2);                                    // (*1)
	if (!(n%2)) count = MOD10(count + unity[n/2]*unity[n/2]);  // (*1)

	double t2 = emscripten_performance_now();

	EM_ASM(document.body.innerHTML += 'Number of strings of length N=' + $0 + ' with UNITY occurring as substring exactly once: ' + $1 + ' (mod 1000000000) (calculated in ' + $2.toFixed(3) + ' msecs)<br>',
		n+5, (int)count, t2-t);
}
