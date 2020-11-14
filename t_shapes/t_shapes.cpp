#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))

static int latticeSize;

// In the orientation that we compute the transforms, the axis-aligned bounding box size
// of the transformed shape is defined by only the two vertices out of all eight
// (+ (0,0) origin).
#define NUM_VERTICES 2
const int vertsX[NUM_VERTICES] = { 2, 3, /*0, 0, 1, 2, 1, 3,*/ };
const int vertsY[NUM_VERTICES] = { 3, 1, /*0, 1, 1, 1, 3, 0,*/ };

uint64_t solve(int x, int y, bool solveScales)
{
	// Transform vectors of the T shape from local basis (x,y),(|-y|,x) to grid space,
	// (0,0) corner implicitly already transformed.
	int w = 0, h = 0;
	for(int i = 0; i < NUM_VERTICES; ++i)
	{
		int X = x * vertsX[i] + y * vertsY[i];
		int Y = y * vertsX[i] + x * vertsY[i];
		w = MAX(w, X);
		h = MAX(h, Y);
	}

	// Calculate how many ways we can position the shape inside the lattice by offsetting
	// (only) translation:
	if (!solveScales)
		return ((uint64_t)MAX(latticeSize-w, 0)) * MAX(latticeSize-h, 0);

	// Tally up the counts when the shape is scaled up to larger sizes in addition to
	// translating:
	uint64_t count = 0;
	for(int i = 1;; ++i)
	{
		int W = latticeSize-w*i;
		int H = latticeSize-h*i;
		if (W <= 0 || H <= 0)
			return count;
		count += W*H;
	}
}

int main()
{
	latticeSize = EM_ASM_INT(return parseInt(location.search.substr(1) || '12345')) + 1;

	double t = emscripten_performance_now();

	// Calculate unrotated positions:
	uint64_t count = 4*(solve(1, 0, true) + solve(1, 1, true)); // Mirror 4* for (i,0),(-i,0),(0,i),(0,-i) and (i,i),(i,-i),(-i,i),(-i,-i)

	// Fit all possible (x,y) basis rotations of the T shape to the lattice grid.
	for(int y = 2;; ++y)
	{
		uint64_t count0 = count;
		for(int x = 1; x < y; ++x)
		{
			uint64_t c = solve(x, y, false);
			if (!c) break; // Sizes are only getting larger after this, if this X size didn't fit, later ones won't either
			count += 8*c; // Mirror 8* for (x,y),(-x,y),(x,-y),(-x,-y) and (y,x),(-y,x),(y,-x),(-y,-x)
		}
		if (count == count0)
			break; // Sizes are only getting larger after this, if this Y size didn't fit, later ones won't either
	}
	double t2 = emscripten_performance_now();

	EM_ASM(document.body.innerHTML += 'Num T shapes in a lattice grid of size ' + ($0-1) + ': ' + $1 + '. (calculated in ' + $2.toFixed(3) + ' msecs)<br>',
		latticeSize, (double)count, t2 - t);
}
