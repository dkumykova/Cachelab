/* 
 * dkumykova-tgsprowl
 * Diana Kumykova and Tyler Sprowl
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
	//need to transpose line by line within each block
	//then go through, if encounter a diagonal save that in a variables and come back to it later; outside
	//of loop
	//separate into blocks of 8x8 first
	int j = 0;
	int k = 0;
	int jj;
	int kk;
	int temp; //stores value for swapping
	//int diagonal; //store diagonal value to deal with later
	//int diagonal_j; //stores j index for diagonal
	//int diagonal_k; //stores k index for diagonal

	//stay within bounds of block
	for (jj = 0; jj < N; jj += N / 4) { //row is 8 max (per block, 32 total
		for (kk = 0; kk < M; kk += M / 4) { //column is 8 max per block, 32 total

			//right now, processes one by one

			for (j = jj; j < jj + N / 4; j++) { //row of block
				for (k = kk; k < kk + M / 4; k++) { //col of block

					if (j == k && jj == kk) { //diagonal case
						continue;
						//diagonal = A[j][k];
						//diagonal_j = j;
						//diagonal_k = k;

					} //else {
					temp = A[j][k]; //transpose
					B[k][j] = temp;
				}
				//after go through whole line, then transpose diagonal case
				if (j == k) {
					temp = A[j][k];
					B[k][j] = temp; //diagonal, j = k

					//B[diagonal_k][diagonal_j] = temp;
				}

			}
		}

	}

}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
	int i, j, tmp;

	for (i = 0; i < N; i++) {
		for (j = 0; j < M; j++) {
			tmp = A[i][j];
			B[j][i] = tmp;
		}
	}

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
	/* Register your solution function */
	registerTransFunction(transpose_submit, transpose_submit_desc);

	/* Register any additional transpose functions */
	registerTransFunction(trans, trans_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
	int i, j;

	for (i = 0; i < N; i++) {
		for (j = 0; j < M; ++j) {
			if (A[i][j] != B[j][i]) {
				return 0;
			}
		}
	}
	return 1;
}

