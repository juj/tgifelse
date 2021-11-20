#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <emscripten.h>

#define N 10000

typedef struct point
{
  int16_t x, y;
} point;

// Sort points from left to right, breaking ties with y coordinate
int cmp(const void *a, const void*b)
{
  int dx = ((const point*)a)->x - ((const point*)b)->x;
  return dx != 0 ? dx : ((const point*)a)->y - ((const point*)b)->y;
}

// Generator from the problem statement
uint64_t g = 289991;
int gen()
{
  g = (g * g) % 45985633ull;
  return g;
}

int main()
{
  // Define an input point array of 3*N points.
#define PTS (3*N)
  point *p = (point*)malloc(sizeof(point)*PTS);

  for(int i = 0; i < N; ++i)
  {
    // First N points form a left side guardband for the left edge of the N*N grid at (-1, y) for all y.
    p[i].x = -1;
    p[i].y = i;
    // Next N points are the input points inside the N*N grid.
    p[N+i].x = gen() % N;
    p[N+i].y = gen() % N;
    // Last N points are a right side guardband for the right edge of the N*N grid at (N, y) for all y.
    p[2*N+i].x = N;
    p[2*N+i].y = i;
  }
  // Sort points ascending based on X coordinate.
  qsort(p, PTS, sizeof(point), cmp);

  // Track the area and edge coordinates of the best found empty rectangle.
  // Coordinates X0 and Y0 are inclusive, X1 and Y1 are exclusive.
  int bestArea = 0;
  int X0, Y0, X1, Y1;

  // General strategy: loop through each pair of points i and j, and span
  // an empty rectangle with i as the left edge and j as the right edge.
  // Fixing the left and right edges will also fix a unique remaining choice
  // for the top and bottom edges.
  // Since we loop through the input point array in sorted order, we can
  // incrementally keep track of what the top and bottom edges will end up
  // being for each of the created rectangles.
  double t0 = emscripten_get_now();
  for(int i = 0; i < PTS; ++i)
  {
    // Left edge is defined by point i at x0.
    int x0 = p[i].x + 1;

    // Incrementally constrain top and bottom edge extents yMin and yMax.
    int yMin = 0;
    int yMax = N;

    // Loop through each possible point for a right edge j.
    int j = i+1;

    // First skip points on the same x coordinate as i (those can't form a
    // right edge pair with this starting point i)
    while(p[j].x == p[i].x) ++j;

    // Then test each subsequent point j as a possible right edge for i.
    for(; j < PTS; ++j)
    {
      // Area of rectangle with left edge at x0, right edge at j, top
      // edge at yMin and bottom edge at yMax.
      int area = (p[j].x-x0)*(yMax-yMin);
      if (area > bestArea)
      {
        bestArea = area;
        X0 = x0; Y0 = yMin; X1 = p[j].x; Y1 = yMax;
      }

      // If this point j is at same vertical y position as the left point i,
      // then we can't span any rectangles with an x coordinate further than j
      // that will also start with point i.
      if (p[j].y == p[i].y)
         break;

      // Retire point j to become either a top edge or a bottom edge for the
      // subsequent rectangles, depending on its y coordinate.
      if (p[j].y < p[i].y) // point j will retire as a top edge constraint?
      {
        // Constrain the yMin extent for the subsequent formed rectangles.
        if (yMin < p[j].y + 1)
        {
          yMin = p[j].y + 1;
          // Loop early out optimization: if we can no longer possibly form a
          // larger rectangle with left edge at x0 even if we extruded right
          // edge all the way out to N, then early out from the inner loop.
          if ((N-x0)*(yMax-yMin) <= bestArea) break;
        }
      }
      else if (yMax > p[j].y)
      {
        // point j will retire as a bottom edge, constrain the yMax extent for
        // the subsequent formed rectangles.
        yMax = p[j].y;
        // Loop early out optimization
        if ((N-x0)*(yMax-yMin) <= bestArea) break;
      }
    }
  }
  double t1 = emscripten_get_now();

  EM_ASM(({document.body.innerText += `Best area: ${$0}, ul=${$1}x${$2}, br=${$3}x${$4}, w*h=${$5}x${$6}. Calculated in ${$7} msecs.`;}),
	bestArea, X0, Y0, X1, Y1, X1-X0, Y1-Y0, t1-t0);

  // Validate correct result:
  for(int i = 0; i < PTS; ++i)
    if (p[i].x >= X0 && p[i].y >= Y0 && p[i].x < X1 && p[i].y < Y1)
        printf("Oops! Incorrect calculation: supposed largest empty rectangle is not empty but contains point %d\n", i);
}
