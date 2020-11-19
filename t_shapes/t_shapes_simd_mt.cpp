#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdint.h>
#include <wasm_simd128.h>
#include <emscripten/threading.h>
#include <emscripten/wasm_worker.h>
#include <emscripten/stack.h>

extern "C" void logResultsMt(int size, double count, double time, int threads);
extern "C" int readInputSize(void);

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))

// In the orientation that we compute the transforms, the axis-aligned bounding box size
// of the transformed shape is defined by only the two vertices out of all eight
// (+ (0,0) origin).
#define NUM_VERTICES 2
const int vertsX[NUM_VERTICES] = { 2, 3, /*0, 0, 1, 2, 1, 3,*/ };
const int vertsY[NUM_VERTICES] = { 3, 1, /*0, 1, 1, 1, 3, 0,*/ };

int latticeSize = readInputSize() + 1;

uint64_t solveScaled(int x, int y)
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

uint64_t solveUnscaled(int x, int y)
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
	return ((uint64_t)MAX(latticeSize-w, 0)) * MAX(latticeSize-h, 0);
}

const v128_t LatticeSize = wasm_i32x4_splat(latticeSize);

uint32_t solveSIMD(v128_t X, v128_t Y)
{
	const v128_t c2 = wasm_i32x4_splat(2);
	const v128_t c3 = wasm_i32x4_splat(3);

	v128_t X0 = wasm_i32x4_add(wasm_i32x4_mul(X, c2), wasm_i32x4_mul(Y, c3));
	v128_t Y0 = wasm_i32x4_add(wasm_i32x4_mul(Y, c2), wasm_i32x4_mul(X, c3));

	v128_t X1 = wasm_i32x4_add(wasm_i32x4_mul(X, c3), Y);
	v128_t Y1 = wasm_i32x4_add(wasm_i32x4_mul(Y, c3), X);

	v128_t w = wasm_u32x4_max(X0, X1);
	v128_t h = wasm_u32x4_max(Y0, Y1);

	const v128_t zero = wasm_i32x4_splat(0);
	v128_t count = wasm_i32x4_mul(wasm_i32x4_max(wasm_i32x4_sub(LatticeSize, w), zero),
	                              wasm_i32x4_max(wasm_i32x4_sub(LatticeSize, h), zero));

	v128_t shuffled = wasm_v32x4_shuffle(count, count, 3, 2, 5, 4);
	count = wasm_i32x4_add(count, shuffled);
	return count[0] + count[1];
}

uint64_t finalCount = 0;
double finalTime = 0;
const int numThreads = 64; // emscripten_navigator_hardware_concurrency();
emscripten_semaphore_t threadReady = EMSCRIPTEN_SEMAPHORE_T_STATIC_INITIALIZER(0);
emscripten_semaphore_t threadStart = EMSCRIPTEN_SEMAPHORE_T_STATIC_INITIALIZER(0);
emscripten_semaphore_t threadDone = EMSCRIPTEN_SEMAPHORE_T_STATIC_INITIALIZER(0);

void work_thread(int threadIdx)
{
	emscripten_semaphore_release(&threadReady, 1);
	emscripten_semaphore_waitinf_acquire(&threadStart, 1);

	// Calculate unrotated positions:
	uint64_t count = 0;

	if (threadIdx == numThreads-1)
		count += 4*(solveScaled(1, 0) + solveScaled(1, 1)); // Mirror 4* for (i,0),(-i,0),(0,i),(0,-i) and (i,i),(i,-i),(-i,i),(-i,-i)

	// Fit all possible (x,y) basis rotations of the T shape to the lattice grid.
	const v128_t y0 = wasm_i32x4_splat(threadIdx+1);
	const v128_t yInc = wasm_i32x4_splat(numThreads);
	const v128_t inc = wasm_i32x4_const(1, 2, 3, 4);
	const v128_t c4 = wasm_i32x4_splat(4);

	for(v128_t y = y0;; y = wasm_i32x4_add(y, yInc))
	{
		uint64_t count0 = count;
		for(v128_t x = wasm_i32x4_add(y, inc);; x = wasm_i32x4_add(x, c4))
		{
			uint64_t c = solveSIMD(x, y);
			if (!c) break; // Sizes are only getting larger after this, if this X size didn't fit, later ones won't either
			count += 8*c; // Mirror 8* for (x,y),(-x,y),(x,-y),(-x,-y) and (y,x),(-y,x),(y,-x),(-y,-x)
		}
		if (count == count0)
			break; // Sizes are only getting larger after this, if this Y size didn't fit, later ones won't either
	}

	emscripten_atomic_add_u64((void*)&finalCount, count);
	emscripten_semaphore_release(&threadDone, 1);
}

void printResults()
{
	logResultsMt(latticeSize, (double)finalCount, finalTime, numThreads);
}

void control_thread()
{
	// Sleep until all threads have loaded up
	emscripten_semaphore_waitinf_acquire(&threadReady, numThreads);

	// Work starts
	double t0 = emscripten_performance_now();

	// Tell all threads to get going
	emscripten_semaphore_release(&threadStart, numThreads);

	// Sleep until all threads have finished
	int x = emscripten_semaphore_waitinf_acquire(&threadDone, numThreads);

	// Work is now done
	finalTime = emscripten_performance_now() - t0;

	emscripten_wasm_worker_post_function_v(EMSCRIPTEN_WASM_WORKER_ID_PARENT, printResults);
}

char threadStack[1024*65];

int main(int argc, char **argv)
{
	int stackSize = sizeof(threadStack) / numThreads;
	for(int i = 0; i < numThreads; ++i)
	{
		emscripten_wasm_worker_t worker = emscripten_create_wasm_worker(threadStack + stackSize*i, stackSize);
		emscripten_wasm_worker_post_function_vi(worker, work_thread, i);
	}
	emscripten_wasm_worker_t worker = emscripten_create_wasm_worker(threadStack + stackSize*numThreads, stackSize);
	emscripten_wasm_worker_post_function_v(worker, control_thread);
}
