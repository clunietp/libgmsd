#include "convolve.h"
#include "matrix.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>

typedef void(*apply_kernel_scanline_fn)(const matrix_data_t**, const matrix_data_t**, const size_t, const size_t, matrix_data_t*);

void apply_kernel_scanline(const matrix_data_t** kernel, const matrix_data_t** src, const size_t ksize, const size_t cols, matrix_data_t* result) {

	size_t c, r, kc;
	const matrix_data_t
		*kr
		, *srcr
		, *srcrc
		;
	matrix_data_t* out;

	for (r = 0; r < ksize; ++r) {
		kr = kernel[r];
		srcr = src[r];
		for (c = 0; c < cols; ++c ) {
			srcrc = srcr + c;
			out = result + c;
			for (kc = 0; kc < ksize; ++kc)
				*out += (srcrc[kc] * kr[kc]);
		}
	}

}

void apply_kernel_scanline_2x2(const matrix_data_t** kernel, const matrix_data_t** src, const size_t unused_, const size_t cols, matrix_data_t* result) {
	size_t c;
	for (c = 0; c < cols; ++c) {
		result[c] = kernel[0][0] * src[0][c] + kernel[0][1] * src[0][c + 1] + kernel[1][0] * src[1][c] + kernel[1][1] * src[1][c + 1];
	}
}

void apply_kernel_scanline_3x3(const matrix_data_t** kernel, const matrix_data_t** src, const size_t unused_, const size_t cols, matrix_data_t* result) {
	size_t c;
	for (c = 0; c < cols; ++c) {
		result[c] = 
			kernel[0][0] * src[0][c] 
			+ kernel[0][1] * src[0][c + 1] 
			+ kernel[0][2] * src[0][c + 2]
			+ kernel[1][0] * src[1][c] 
			+ kernel[1][1] * src[1][c + 1]
			+ kernel[1][2] * src[1][c + 2]
			+ kernel[2][0] * src[2][c]
			+ kernel[2][1] * src[2][c + 1]
			+ kernel[2][2] * src[2][c + 2]
			;
	}
}

bool conv2(matrix_t* out, const matrix_t* m, const kernel_t* k, const char* type ) {

#define MAX_KERNEL_SIZE 8

	/* pointers to rows of source */
	const matrix_data_t* src_ptrs[MAX_KERNEL_SIZE] = { NULL };

	/* pointers to rows of kernel */
	const matrix_data_t* k_ptrs[MAX_KERNEL_SIZE] = { NULL };

	size_t r, kr;
	matrix_data_t* data_ptr = NULL;

	const size_t
		ksize = kernel_size(k)
		, pad_size = ksize - 1
		, k_offset = (ksize % 2 == 0) ? 0 : ksize / 2
		;

	matrix_t 
		temp = { 0 }
		, padded = { 0 }
	;
	bool result = false;

	kernel_t k_180 = { 0 };
	apply_kernel_scanline_fn pf_apply;

	/* check input */
	if (
		(ksize > MAX_KERNEL_SIZE)	/* not implemented/supported */
		|| (strcmp(type, "same") != 0)
		|| !m
		|| !out
		)
		goto CLEANUP;

	/* setup */	
	if (
		!matrix_rotate(&k_180, kernel_matrix(k), 180)	/* 180 degree kernel rotate prior to conv2 */
		|| !matrix_pad(&padded, m, pad_size, pad_size)	/* copy/resize input matrix to size that can be used inside apply_kernel without branching, then crop at end*/
		|| !matrix_init(&temp, matrix_rows(&padded), matrix_cols(&padded))
		)
		goto CLEANUP;

	/* set up kernel row ptrs */
	for (kr = 0; kr < ksize; ++kr)
		k_ptrs[kr] = matrix_row(kernel_matrix(&k_180), kr);

	/* choose apply kernel fn based on ksize, using loop-unrolled versions for select kernel sizes */
	switch (ksize) {
	case 2:
		pf_apply = apply_kernel_scanline_2x2;
		break;
	case 3:
		pf_apply = apply_kernel_scanline_3x3;
		break;
	default:
		pf_apply = apply_kernel_scanline;
	}

	/* start convolution */
	for (r = 0; r < matrix_rows(&padded) - pad_size; ++r ) {

		/* set up src row ptrs */
		for (kr = 0; kr < ksize; ++kr) {
			src_ptrs[kr] = matrix_row(&padded, r + kr);
		}

		/* row where the result will go */
		data_ptr = matrix_row(&temp, r + k_offset) + k_offset; 

		/* apply kernel */
		pf_apply(
			k_ptrs
			, src_ptrs
			, ksize
			, matrix_cols(&padded)-pad_size
			, data_ptr
		);

	};	/* for r */

	/* "same" kernel:  crop result */
	result = matrix_crop( out, &temp, matrix_rows(m), matrix_cols(m), pad_size, pad_size);
CLEANUP:
	kernel_destroy(&k_180);
	matrix_destroy(&temp);
	matrix_destroy(&padded);
	return result;
}