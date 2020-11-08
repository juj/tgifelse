#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <emscripten.h>
#include <emscripten/html5.h>

// Possible knight moves. Play out the moves on a 16x16 chess board so that 3 bits are stored for X&Y positions,
// and excess 4th bit stores whether we went out of bounds on a move. This way need 16 bits to store a board state.
#define NUM_MOVES 8
#define RIGHT2_UP1 (-16 + 2)
#define RIGHT2_DOWN1 (16 + 2)
#define DOWN2_RIGHT1 (32 + 1)
#define DOWN2_LEFT1 (32 - 1)
#define LEFT2_DOWN1 (16 - 2)
#define LEFT2_UP1 (-16 - 2)
#define UP2_LEFT1 (-32 - 1)
#define UP2_RIGHT1 (-32 + 1)

const int16_t moves[NUM_MOVES] = { RIGHT2_UP1, RIGHT2_DOWN1, DOWN2_RIGHT1, DOWN2_LEFT1, LEFT2_DOWN1, LEFT2_UP1, UP2_LEFT1, UP2_RIGHT1 };

// knight position takes up 8 bits: 0YYY 0XXX.
#define POS(x, y) ((((uint16_t)(y)) << 4) | ((uint16_t)(x)))
// Extract X coordinate from position field (-----XXX)
#define GET_X(pos) (((uint16_t)(pos)) & 0x0F)
// Extract Y coordinate from position field (-YYY----)
#define GET_Y(pos) (((uint16_t)(pos)) >> 4)
// Tests if XY coordinate is a valid board position (within [0,7] range)
#define IS_INVALID(pos) (((uint16_t)(pos)) & 0x88)

// Builds a board state 0YYY 0XXX 0yyy 0xxx where XY are black knight position, and xy are white knight position.
#define STATE(whitePos, blackPos) (((uint16_t)(whitePos)) | (((uint16_t)(blackPos)) << 8))

int16_t abs(int16_t x)
{
	return x >= 0 ? x : -x;
}

int main()
{
	// Iterate move pairs from src probabilities to dst.
	// Of these 4096 board states are actually used, but need padding for full 16-bit state.
	double p1[65536] = {};
	double p2[65536] = {};

	double *src = p1;
	double *dst = p2;

	// Initial board state with knights in the corners has full probability.
	src[STATE(POS(7,7), POS(0,0))] = 1.0;

	// Probability that white knight has been captured.
	double captureProbability = 0.0;

	int numMovePairs = EM_ASM_INT(return parseInt(location.search.substr(1) || '32'));

	uint16_t validMoves[NUM_MOVES];

	double t = emscripten_performance_now();
	for(int n = 0; n < numMovePairs; ++n)
	{
		// White moves:
		for(int whiteY = 0; whiteY < 8; ++whiteY) for(int whiteX = 0; whiteX < 8; ++whiteX)
		{
			uint16_t whitePos = POS(whiteX, whiteY);

			// Count number of valid moves for white
			int numValidMoves = 0;
			for(int move = 0; move < NUM_MOVES; ++move)
			{
				uint16_t whiteNewPos = whitePos + moves[move];
				if (!IS_INVALID(whiteNewPos))
					validMoves[numValidMoves++] = moves[move]; // This possibly includes white capturing black, will be filtered out in inner loop.
			}

			for(int blackY = 0; blackY < 8; ++blackY) for(int blackX = 0; blackX < 8; ++blackX)
			{
				// This 4x nested loop over knight positions would be faster with PDEP instruction, but not currently
				// available in Wasm (https://github.com/WebAssembly/design/issues/1389)
				uint16_t blackPos = POS(blackX, blackY);
				double srcProbability = src[STATE(whitePos, blackPos)];
				if (!srcProbability) // Skip if this board state has not occurred
					continue;

				int numValidMovesWithoutCapture = numValidMoves - (int)(abs((whiteX-blackX)*(whiteY-blackY)) == 2);

				// Take each move with uniform probability
				double dstProbability = srcProbability / numValidMovesWithoutCapture;

				// Distribute probability to possible moves
				for(int move = 0; move < numValidMoves; ++move)
				{
					uint16_t whiteNewPos = whitePos + validMoves[move];
					if (whiteNewPos != blackPos) // white can't capture black
						dst[STATE(whiteNewPos, blackPos)] += dstProbability;
				}
			}
		}

		double *tmp = src;
		src = dst;
		dst = tmp;
		memset(dst, 0, sizeof(p1));

		// Black moves:
		for(int blackY = 0; blackY < 8; ++blackY) for(int blackX = 0; blackX < 8; ++blackX)
		{
			uint16_t blackPos = POS(blackX, blackY);

			// Count number of valid moves for black
			int numValidMoves = 0;
			for(int move = 0; move < NUM_MOVES; ++move)
			{
				uint16_t blackNewPos = blackPos + moves[move];
				if (!IS_INVALID(blackNewPos)) // black can capture white
					validMoves[numValidMoves++] = moves[move];
			}

			for(int whiteY = 0; whiteY < 8; ++whiteY) for(int whiteX = 0; whiteX < 8; ++whiteX)
			{
				uint16_t whitePos = POS(whiteX, whiteY);
				double srcProbability = src[STATE(whitePos, blackPos)];
				if (!srcProbability) // Skip if this board state has not occurred
					continue;

				// Take each move with uniform probability
				double dstProbability = srcProbability / numValidMoves;

				// Distribute probability to possible moves
				for(int move = 0; move < numValidMoves; ++move)
				{
					uint16_t blackNewPos = blackPos + validMoves[move];
					if (blackNewPos == whitePos) // black can capture white
						captureProbability += dstProbability;
					else
						dst[STATE(whitePos, blackNewPos)] += dstProbability;
				}
			}
		}

		tmp = src;
		src = dst;
		dst = tmp;
		memset(dst, 0, sizeof(p1));
	}
	double t2 = emscripten_performance_now();

	EM_ASM(document.body.innerHTML += 'Probability that white is still alive after ' + $0 + ' move pairs: ' + $1.toFixed(8) + '. (calculated in ' + $2.toFixed(3) + ' msecs)<br>',
		numMovePairs, 1.0 - captureProbability, t2 - t);
}
