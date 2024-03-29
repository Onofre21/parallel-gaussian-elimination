/*
 * Gaussian_Block_Parallel.c
 *
 *  Created on: Apr 27, 2009
 *      Author: yzhang8
 */

#include "Gaussian.h"
#include "Gaussian_Block_Parallel.h"

inline int min(int a, int b);
void* compute_row_block(void* s);

void gaussian_elimination_block_parallel() {
	time_t time_sec_start, time_sec_finish;
	time_sec_start = time(NULL);
	pthread_t* p_threads = malloc(thread_num * sizeof(pthread_t));
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	thread_row_range* thread_arg = malloc(thread_num * sizeof(thread_row_range));
	int i, j, thread_first_row, thread_row_size, useful_thread_num;
	double *row_swap = malloc(size * sizeof(double)), b_swap;
	char buffer[40];
	for (col = 0; col < size - 1; col++) {// go through each column
		int pivot_row = col;
		for (i = col + 1; i < size; i++) {//find the largest
			if (matrix_A[pivot_row][col] < matrix_A[i][col]) {
				pivot_row = i;
			}
		}
		if (pivot_row != col) {//exchange rows
			b_swap = vector_B[col];
			vector_B[col] = vector_B[pivot_row];
			vector_B[pivot_row] = b_swap;
			for (j = col; j < size; j++) {
				row_swap[j] = matrix_A[col][j];
				matrix_A[col][j] = matrix_A[pivot_row][j];
				matrix_A[pivot_row][j] = row_swap[j];
			}
		}
		useful_thread_num = (size - col - 1) / thread_row_threshold + 1;//calculate how many threads should be used
		if (useful_thread_num > thread_num) {
			useful_thread_num = thread_num;
		}
		thread_first_row = col + 1;
		thread_row_size = (size - col - 1) / useful_thread_num;
		for (i = 0; i < useful_thread_num; i++) {
			thread_arg[i].first_row = thread_first_row;
			thread_arg[i].thread_id = i;
			if (i != useful_thread_num - 1) {//not the last thread
				thread_arg[i].last_row = thread_first_row + thread_row_size - 1;
				thread_first_row = thread_arg[i].last_row + 1;
			} else {
				thread_arg[i].last_row = size - 1;
			}
		}

		for (i = 0; i < useful_thread_num; i++) {//run threads
			pthread_create(&p_threads[i], &attr, compute_row_block, (void*) &thread_arg[i]);
		}
		for (i = 0; i < useful_thread_num; i++) {//wait for all thread to complete calculate the rows
			pthread_join(p_threads[i], NULL);
		}
		sprintf(buffer, "\nGaussian after proccess col %d\n ", col);
		print_Gaussian(buffer);
	}
	time_sec_finish = time(NULL);
	printf("\nGaussian elimination used %d sec\n", (int) (time_sec_finish - time_sec_start));
	free(p_threads);
	free(thread_arg);
}

void* compute_row_block(void* s) {
	thread_row_range* range = (thread_row_range*) s;
	int row, j, jj, jsize, rsize, rr;
	double* pivot_row = malloc(size * sizeof(double));
	for (j = col; j < size; j++) {
		pivot_row[j] = matrix_A[col][j];
	}
	double* row_ratio = malloc(size * sizeof(double));
	for (row = range->first_row; row <= range->last_row; row++) {
		row_ratio[row] = matrix_A[row][col] / pivot_row[col];
	}
	for (row = range->first_row; row <= range->last_row; row += 50) {
		rsize = min(row + 50, range->last_row+1);
		for (j = col; j < size; j += 50) {
			jsize = min(j + 50, size);
			for (rr = row; rr < rsize; rr++) {
				for (jj = j; jj < jsize; jj++) {
					matrix_A[rr][jj] -= pivot_row[jj] * row_ratio[rr];
				}
			}
		}
	}
	for (row = range->first_row; row <= range->last_row; row++) {
		vector_B[row] -= vector_B[col] * row_ratio[row];
	}
	free(pivot_row);
	free(row_ratio);
	pthread_exit(0);
}

inline int min(int a, int b) {
	return ((a > b) ? b : a);
}
