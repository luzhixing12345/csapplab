/*
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
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32) {
        int i, j, k, l;
        int t0, t1, t2, t3, t4, t5, t6, t7;
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                if (i != j) {
                    for (k = i; k < i + 8; k++) {
                        for (l = j; l < j + 8; l++) {
                            B[l][k] = A[k][l];
                        }
                    }
                } else {
                    for (k = i; k < i + 8; k++) {
                        t0 = A[k][j];
                        t1 = A[k][j + 1];
                        t2 = A[k][j + 2];
                        t3 = A[k][j + 3];
                        t4 = A[k][j + 4];
                        t5 = A[k][j + 5];
                        t6 = A[k][j + 6];
                        t7 = A[k][j + 7];
                        B[k][j] = t0;
                        B[k][j + 1] = t1;
                        B[k][j + 2] = t2;
                        B[k][j + 3] = t3;
                        B[k][j + 4] = t4;
                        B[k][j + 5] = t5;
                        B[k][j + 6] = t6;
                        B[k][j + 7] = t7;
                    }
                    for (k = i; k < i + 8; k++) {
                        for (l = j + (k - i + 1); l < j + 8; l++) {
                            if (k != l) {
                                t0 = B[k][l];
                                B[k][l] = B[l][k];
                                B[l][k] = t0;
                            }
                        }
                    }
                }
            }
        }
    } else if (M == 64) {
        int i, j, k, l;
        int a0, a1, a2, a3, a4, a5, a6, a7;
        // 优先处理对角线block
        for (j = 0; j < N; j += 8) {
            // 对于第一行的block，借用矩阵选择第一行第二个block
            // 其余都选择每一行的第一个block
            if (j == 0)
                i = 8;
            else
                i = 0;
            // 1. 将A矩阵下方4x8移动到借用矩阵位置 - 上半部分
            for (k = j; k < j + 4; ++k) {
                a0 = A[k + 4][j + 0];
                a1 = A[k + 4][j + 1];
                a2 = A[k + 4][j + 2];
                a3 = A[k + 4][j + 3];
                a4 = A[k + 4][j + 4];
                a5 = A[k + 4][j + 5];
                a6 = A[k + 4][j + 6];
                a7 = A[k + 4][j + 7];
                B[k][i + 0] = a0;
                B[k][i + 1] = a1;
                B[k][i + 2] = a2;
                B[k][i + 3] = a3;
                B[k][i + 4] = a4;
                B[k][i + 5] = a5;
                B[k][i + 6] = a6;
                B[k][i + 7] = a7;
            }
            // 2. 借用矩阵内部两个4x4转置
            for (k = 0; k < 4; ++k) {
                for (l = k + 1; l < 4; ++l) {
                    a0 = B[j + k][i + l];
                    B[j + k][i + l] = B[j + l][i + k];
                    B[j + l][i + k] = a0;
                    a0 = B[j + k][i + l + 4];
                    B[j + k][i + l + 4] = B[j + l][i + k + 4];
                    B[j + l][i + k + 4] = a0;
                }
            }
            // 3. 将A上方4x8移动到B上方
            for (k = j; k < j + 4; ++k) {
                a0 = A[k][j + 0];
                a1 = A[k][j + 1];
                a2 = A[k][j + 2];
                a3 = A[k][j + 3];
                a4 = A[k][j + 4];
                a5 = A[k][j + 5];
                a6 = A[k][j + 6];
                a7 = A[k][j + 7];
                B[k][j + 0] = a0;
                B[k][j + 1] = a1;
                B[k][j + 2] = a2;
                B[k][j + 3] = a3;
                B[k][j + 4] = a4;
                B[k][j + 5] = a5;
                B[k][j + 6] = a6;
                B[k][j + 7] = a7;
            }
            // 4. B矩阵上方两个4x4转置
            for (k = j; k < j + 4; ++k) {
                for (l = k + 1; l < j + 4; ++l) {
                    a0 = B[k][l];
                    B[k][l] = B[l][k];
                    B[l][k] = a0;
                    a0 = B[k][l + 4];
                    B[k][l + 4] = B[l][k + 4];
                    B[l][k + 4] = a0;
                }
            }
            // 5. B矩阵的右上和转置矩阵的左上交换
            for (k = 0; k < 4; ++k) {
                a0 = B[j + k][j + 4];
                a1 = B[j + k][j + 5];
                a2 = B[j + k][j + 6];
                a3 = B[j + k][j + 7];
                B[j + k][j + 4] = B[j + k][i + 0];
                B[j + k][j + 5] = B[j + k][i + 1];
                B[j + k][j + 6] = B[j + k][i + 2];
                B[j + k][j + 7] = B[j + k][i + 3];
                B[j + k][i + 0] = a0;
                B[j + k][i + 1] = a1;
                B[j + k][i + 2] = a2;
                B[j + k][i + 3] = a3;
            }
            // 6. 将借用矩阵移动到B矩阵下方
            for (k = 0; k < 4; ++k) {
                B[j + k + 4][j + 0] = B[j + k][i + 0];
                B[j + k + 4][j + 1] = B[j + k][i + 1];
                B[j + k + 4][j + 2] = B[j + k][i + 2];
                B[j + k + 4][j + 3] = B[j + k][i + 3];
                B[j + k + 4][j + 4] = B[j + k][i + 4];
                B[j + k + 4][j + 5] = B[j + k][i + 5];
                B[j + k + 4][j + 6] = B[j + k][i + 6];
                B[j + k + 4][j + 7] = B[j + k][i + 7];
            }
            // 处理非对角线block
            for (i = 0; i < M; i += 8) {
                if (i == j) {
                    continue;
                } else {
                    // 1. 将A矩阵上半部分移动到B上半部分
                    //    分为两块 分别转置
                    for (k = i; k < i + 4; ++k) {
                        a0 = A[k][j + 0];
                        a1 = A[k][j + 1];
                        a2 = A[k][j + 2];
                        a3 = A[k][j + 3];
                        a4 = A[k][j + 4];
                        a5 = A[k][j + 5];
                        a6 = A[k][j + 6];
                        a7 = A[k][j + 7];
                        B[j + 0][k] = a0;
                        B[j + 1][k] = a1;
                        B[j + 2][k] = a2;
                        B[j + 3][k] = a3;
                        B[j + 0][k + 4] = a4;
                        B[j + 1][k + 4] = a5;
                        B[j + 2][k + 4] = a6;
                        B[j + 3][k + 4] = a7;
                    }
                    // 2. B矩阵右上部分移动到B矩阵左下，转置
                    //    A矩阵左下部分移动到B矩阵右上，转置
                    for (l = j; l < j + 4; ++l) {
                        a0 = A[i + 4][l];
                        a1 = A[i + 5][l];
                        a2 = A[i + 6][l];
                        a3 = A[i + 7][l];
                        a4 = B[l][i + 4];
                        a5 = B[l][i + 5];
                        a6 = B[l][i + 6];
                        a7 = B[l][i + 7];
                        B[l][i + 4] = a0;
                        B[l][i + 5] = a1;
                        B[l][i + 6] = a2;
                        B[l][i + 7] = a3;
                        B[l + 4][i + 0] = a4;
                        B[l + 4][i + 1] = a5;
                        B[l + 4][i + 2] = a6;
                        B[l + 4][i + 3] = a7;
                    }
                    // 3. A矩阵右下部分移动到B矩阵右下，转置
                    for (k = i + 4; k < i + 8; ++k) {
                        a0 = A[k][j + 4];
                        a1 = A[k][j + 5];
                        a2 = A[k][j + 6];
                        a3 = A[k][j + 7];
                        B[j + 4][k] = a0;
                        B[j + 5][k] = a1;
                        B[j + 6][k] = a2;
                        B[j + 7][k] = a3;
                    }
                }
            }
        }
    } else {
        // M = 61 N = 67
        int i, j, t0, t1, t2, t3, t4, t5, t6, t7;
        int k = N / 8 * 8;
        int l = M / 8 * 8;
        for (j = 0; j < l; j += 8)
            for (i = 0; i < k; ++i) {
                t0 = A[i][j];
                t1 = A[i][j + 1];
                t2 = A[i][j + 2];
                t3 = A[i][j + 3];
                t4 = A[i][j + 4];
                t5 = A[i][j + 5];
                t6 = A[i][j + 6];
                t7 = A[i][j + 7];
                B[j][i] = t0;
                B[j + 1][i] = t1;
                B[j + 2][i] = t2;
                B[j + 3][i] = t3;
                B[j + 4][i] = t4;
                B[j + 5][i] = t5;
                B[j + 6][i] = t6;
                B[j + 7][i] = t7;
            }
        for (i = k; i < N; ++i)
            for (j = l; j < M; ++j) {
                t0 = A[i][j];
                B[j][i] = t0;
            }
        for (i = 0; i < N; ++i)
            for (j = l; j < M; ++j) {
                t0 = A[i][j];
                B[j][i] = t0;
            }
        for (i = k; i < N; ++i)
            for (j = 0; j < M; ++j) {
                t0 = A[i][j];
                B[j][i] = t0;
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
    // registerTransFunction(trans, trans_desc);
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
