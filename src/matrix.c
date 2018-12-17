#include "matrix.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <math.h>
#include <malloc.h>	/* _mm_malloc/_mm_free.   todo:  need more portable solution? */

#define MALLOC_ALIGN_DEFAULT 16

/* memory malloc; get aligned if we can */
void* malloc_( size_t sz, size_t align ) {	
	void* result = NULL;
#ifdef _mm_malloc
	result = _mm_malloc(sz, align);
#else
	result = malloc( sz );
#endif
	return result;
}

/* free potentially-aligned memory; memory must have been created with malloc_ */
void free_(void* ptr ) {
	if (!ptr)
		return;
#ifdef _mm_free
	_mm_free(ptr);
#else
	free(ptr);
#endif
}

/* verifies the dimensions of 'cmp' are at least as big as 'target' */
bool dimensions_check_(const matrix_t* target, const matrix_t* cmp) {
	return
		target
		&& cmp
		&& matrix_size(target) > 0
		&& matrix_cols(cmp) >= matrix_cols(target)
		&& matrix_rows(cmp) >= matrix_rows(target)
		;
}

bool check_( matrix_t* out, const matrix_t* in ) {

	if ( !out 
		|| !in
		|| (out != in && !matrix_reset(out, matrix_rows(in), matrix_cols(in)))
	) {
		assert(false);
		return false;
	}
	return true;
}

bool check2_(matrix_t* out, const matrix_t* in1, const matrix_t* in2) {

	if (
		!out
		|| !in1
		|| !in2
		|| !dimensions_check_(in1, in2)
		|| (out == in2 && !dimensions_check_(in2, in1))
		|| (out != in1 && out != in2 && !matrix_reset(out, matrix_rows(in1), matrix_cols(in1)))
		) {
		assert(false);
		return false;
	}
	return true;
}

/* matrix initialization with rows, cols, data; returns flag of success */
bool matrix_init( matrix_t* m, const size_t rows, const size_t cols) {

	if (!m || !rows || !cols )
		return false;

	matrix_init_default(m);
	m->cols = cols;
	m->rows = rows;
	m->data = (matrix_data_t*)malloc_(matrix_size(m) * sizeof(matrix_data_t), MALLOC_ALIGN_DEFAULT);

	if (!m->data)
		return false;
	
	matrix_clear(m);
	return true;
}

/* matrix malloc with dimensions, returns NULL on failure */
matrix_t* matrix_malloc(const size_t rows, const size_t cols) {

	matrix_t* result = (matrix_t*)malloc(sizeof(matrix_t));
	if (!result)
		return NULL;

	if (!matrix_init(result, rows, cols)) {
		free(result);
		return NULL;
	}
	return result;
}

bool matrix_reset(matrix_t* m, const size_t rows, const size_t cols) {

	if (!m)
		return false;

	/* re-task existing matrix if we can */
	if ( m->data && !matrix_is_submatrix(m) && matrix_size(m) >= rows * cols) {
		m->rows = rows;
		m->cols = cols;
		matrix_clear(m);
		return true;
	}
	
	matrix_destroy(m);
	return matrix_init(m, rows, cols);
}

void matrix_copy_data(matrix_data_t* out, const matrix_t* in) {

	size_t r = 0;
	const matrix_data_t* pSrc;
	matrix_data_t* pDst;

	for (r = 0; r < matrix_rows(in); ++r) {
		pSrc = matrix_crow(in, r);
		pDst = out + matrix_cols(in) * r;
		memcpy(pDst, pSrc, matrix_cols(in) * sizeof(matrix_data_t));
	}
}

/* internal transpose/rotate fns */
void transpose(const matrix_data_t* src, matrix_data_t* dst, int rows, int cols);
void transpose_inplace(matrix_data_t* data, int n);
void rotate(int direction, const matrix_data_t* src, matrix_data_t* dst, int rows, int cols);
void rotate_inplace(int direction, matrix_data_t* data, int n);
void reverse_rows(matrix_data_t* data, int rows, int cols);
void reverse_cols(matrix_data_t* data, int rows, int cols);

bool matrix_pad(matrix_t* out, const matrix_t* m, const size_t padX, const size_t padY ) {

	size_t r = 0;
	const matrix_data_t* pSrc;
	matrix_data_t* pDst;

	matrix_t temp = { 0 };
	
	if (!matrix_reset(&temp, matrix_rows(m) + padY * 2, matrix_cols(m) + padX * 2))
		return false;

	/* copy data from src to dst at offset */
	for (r = 0; r < matrix_rows(m); ++r) {
		pSrc = matrix_crow(m, r);
		pDst = matrix_row(&temp, r + padY) + padX;
		memcpy(pDst, pSrc, matrix_cols(m) * sizeof(matrix_data_t));
	}

	matrix_swap(&temp, out);
	matrix_destroy(&temp);

	return true;
}

bool matrix_crop(matrix_t* out, const matrix_t* m, const size_t rows, const size_t cols, const size_t start_col_index, const size_t start_row_index) {
	
	size_t
		r_src
		, r_dst
		;
	const matrix_data_t* pSrc;
	matrix_data_t* pDst;
	matrix_t temp = { 0 };
	
	if (!matrix_reset(&temp, rows, cols))
		return false;

	/* copy data from src to dst at offset */
	for (r_dst = 0, r_src = start_row_index; r_dst < rows; ++r_src, ++r_dst) {
		pSrc = matrix_crow(m, r_src) + start_col_index;
		pDst = matrix_row(&temp, r_dst);
		memcpy(pDst, pSrc, cols * sizeof(matrix_data_t));
	}

	matrix_swap(&temp, out);
	matrix_destroy(&temp);

	return true;
}

void matrix_submatrix(matrix_t* out, const matrix_t* in, const size_t rows, const size_t cols, const size_t start_row, const size_t start_col ) {
	out->cols = cols;
	out->rows = rows;
	out->parent = in;
	out->start_col = start_col;
	out->data = in->data;
}

void matrix_rotate_inplace( matrix_t* m, int direction) {
	assert(direction == 90 || direction == -90 || direction == 180 || direction == -180);
	assert(matrix_rows(m) == matrix_cols(m));
	rotate_inplace(direction, matrix_data(m), (int)matrix_rows(m));
}


bool matrix_rotate(matrix_t* out, const matrix_t* in, int direction ) {

	assert(direction == 90 || direction == -90 || direction == 180 || direction == -180);

	if ( !check_(out,in) )
		return false;
	
	rotate( direction, matrix_cdata( in ), matrix_data( out ), (int)matrix_rows(in ), (int)matrix_cols(in ));

	return true;
}

bool matrix_transpose( matrix_t* out, const matrix_t* m ) {

	matrix_t temp = { 0 };
	
	if (!matrix_reset(&temp, matrix_cols(m), matrix_rows(m)))/* swapping cols, rows from input */
		return false;
	
	transpose( matrix_cdata( m ), matrix_data(&temp), (int)matrix_rows(m), (int)matrix_cols(m));

	matrix_swap(&temp, out);
	matrix_destroy(&temp);

	return true;
}

void matrix_assign_d(matrix_t* m, const double* src) {
	
	size_t r, c;
	matrix_data_t* pMat;

	for (r = 0; r < matrix_rows(m); ++r) {
		pMat = matrix_row(m, r);
		for (c = 0; c < matrix_cols(m); ++c, ++pMat, ++src)
			*pMat = (matrix_data_t)*src;
	}
}

void matrix_assign(matrix_t* m, const matrix_data_t* src) {

	size_t r, c;
	matrix_data_t* pMat;

	for (r = 0; r < matrix_rows(m); ++r) {
		pMat = matrix_row(m, r);
		for (c = 0; c < matrix_cols(m); ++c, ++pMat, ++src)
			*pMat = *src;
	}
}

void matrix_assign_u8(matrix_t* m, const uint8_t* src ) {

	size_t r, c;
	matrix_data_t* pMat;

	for (r = 0; r < matrix_rows(m); ++r) {
		pMat = matrix_row(m, r);
		for (c = 0; c < matrix_cols(m); ++c, ++pMat, ++src)
			*pMat = (matrix_data_t)*src;
	}
}

double matrix_sum(const matrix_t* m) {

	size_t r, c;
	const matrix_data_t* pSrc;
	double result = 0.;

	for (r = 0; r < matrix_rows(m); ++r) {
		pSrc = matrix_crow(m, r);
		for (c = 0; c < matrix_cols(m); ++c, ++pSrc)
			result += *pSrc;
	}

	return result;
}

double matrix_variance_p(const matrix_t* m) {
	/* 
	http://www.sanfoundry.com/c-program-mean-variance-standard-deviation/ 
	*/

	size_t r, c;
	const matrix_data_t* pSrc;
	double var_sum = 0.;
	const size_t sz = matrix_size(m);
	const double
		sum = matrix_sum(m)
		, avg = sum / (double)sz
		;

	for (r = 0; r < matrix_rows(m); ++r) {
		pSrc = matrix_crow(m, r);
		for (c = 0; c < matrix_cols(m); ++c, ++pSrc)
			var_sum += pow(*pSrc - avg, 2.);
	}

	return var_sum / (double)(sz - 1); /* subtracting 1 for pvar */
}

double matrix_stdev_p(const matrix_t* m) {
	return sqrt(matrix_variance_p(m));
}

void matrix_cwise_add(matrix_t* out, const matrix_t* in1, const matrix_t* in2) {

	size_t r, c;

	matrix_data_t* pOut;
	const matrix_data_t
		*pSrc1
		, *pSrc2
		;

	if (!check2_(out, in1, in2))
		return;

	for (r = 0; r < matrix_rows(out); ++r) {
		pSrc1 = matrix_crow(in1, r);
		pSrc2 = matrix_crow(in2, r);
		pOut = matrix_row(out, r);

		for (c = 0; c < matrix_cols(out); ++c, ++pSrc1, ++pSrc2, ++pOut)
			*pOut = *pSrc1 + *pSrc2;
	}

}

void matrix_cwise_add_scalar(matrix_t* out, const matrix_t* m, const matrix_data_t scalar) {

	size_t r, c;
	const matrix_data_t* pSrc;
	matrix_data_t* pOut;
	
	if (!check_(out, m))
		return;
	
	for (r = 0; r < matrix_rows(m); ++r) {
		pSrc = matrix_crow(m, r);
		pOut = matrix_row(out, r);

		for (c = 0; c < matrix_cols(m); ++c, ++pSrc, ++pOut)
			*pOut = *pSrc + scalar;
	}
	
}

void matrix_cwise_scale( matrix_t* out, const matrix_t* in1, const matrix_t* in2) {
	
	size_t r, c;
	const matrix_data_t
		*pIn1, *pIn2
		;
	matrix_data_t* pOut;

	if (!check2_(out, in1, in2))
		return;
	
	for (r = 0; r < matrix_rows(out); ++r) {
		pIn1 = matrix_crow(in1, r);
		pIn2 = matrix_crow(in2, r);
		pOut = matrix_row(out, r);
		for (c = 0; c < matrix_cols(out); ++c, ++pIn1, ++pIn2, ++pOut)
			*pOut = *pIn1 * *pIn2;
	}

}

double matrix_cwise_scale_sum( const matrix_t* lhs, const matrix_t* rhs ) {
	
	size_t r,c;
	double result = { 0 };
	const matrix_data_t
		*pLhs
		, *pRhs
		;

	if (!dimensions_check_(lhs, rhs)) {
		assert(false);
		return result;
	}

	for (r = 0; r < matrix_rows(lhs); ++r) {
		pLhs = matrix_crow(lhs, r);
		pRhs = matrix_crow(rhs, r);
		for (c = 0; c < matrix_cols(lhs); ++c, ++pLhs, ++pRhs)
			result += *pLhs * *pRhs;
	}

	return result;
}

void matrix_cwise_divide(matrix_t* out, const matrix_t* in1, const matrix_t* in2) {

	size_t r, c;
	const matrix_data_t
		*pSrc1
		, *pSrc2
		;
	matrix_data_t* pOut;

	if (!check2_(out, in1, in2))
		return;

	for (r = 0; r < matrix_rows(out); ++r) {
		pSrc1 = matrix_crow(in1, r);
		pSrc2 = matrix_crow(in2, r);
		pOut = matrix_row(out, r);

		for (c = 0; c < matrix_cols(out); ++c, ++pSrc1, ++pSrc2, ++pOut)
			*pOut = *pSrc1 / *pSrc2;
	}

}

void matrix_cwise_square( matrix_t* out, const matrix_t* m) {
	assert(m);
	assert(out);
	if (!m || !out ) return;
	matrix_cwise_scale( out, m, m);
}

void matrix_cwise_scale_scalar(matrix_t* out, const matrix_t* in, const matrix_data_t scalar) {

	size_t r, c;
	const matrix_data_t* pSrc;
	matrix_data_t* pDst;

	if (!check_(out, in))
		return;
	
	for (r = 0; r < matrix_rows(out); ++r) {
		pSrc = matrix_crow(in, r);
		pDst = matrix_row(out, r);
		for (c = 0; c < matrix_cols(out); ++c, ++pSrc, ++pDst)
			*pDst = *pSrc * scalar;
	}
}

void matrix_cwise_sqrt(matrix_t* out, const matrix_t* in) {

	size_t r, c;
	matrix_data_t* pOut;
	const matrix_data_t* pSrc;

	if (!check_(out, in))
		return;
	
	for (r = 0; r < matrix_rows(out); ++r) {
		pSrc = matrix_crow(in, r);
		pOut = matrix_row(out, r);
		for (c = 0; c < matrix_cols(out); ++c, ++pSrc, ++pOut)
			*pOut = sqrtf(*pSrc);
	}
}

void matrix_destroy(matrix_t* m) {

	if (!m)
		return;

	if (!m->parent && m->data) {
		free_(m->data);
		m->data = NULL;
	}
}

void matrix_swap(matrix_t* lhs, matrix_t* rhs) {
	if (!lhs || !rhs)
		return;
	matrix_t temp = *lhs;
	*lhs = *rhs;
	*rhs = temp;
}

/*
Matrix transpose & rotate (+/-90, +/-180)
Supports both 2D arrays and 1D pointers with logical rows/cols
Supports square and non-square matrices, has in-place and copy features
See tests for examples of usage
tested gcc -std=c90 -Wall -pedantic, MSVC17
*/

/*
Reverse values in place of each row in 2D matrix data[rows][cols] or in 1D pointer with logical rows/cols
[A B C]	->	[C B A]
[D E F]		[F E D]
*/
void reverse_rows(matrix_data_t* data, int rows, int cols) {

	int r, c;
	matrix_data_t temp;
	matrix_data_t* pRow = NULL;

	for (r = 0; r < rows; ++r) {
		pRow = (data + r * cols);
		for (c = 0; c < (int)(cols / 2); ++c) { /* explicit truncate */
			temp = pRow[c];
			pRow[c] = pRow[cols - 1 - c];
			pRow[cols - 1 - c] = temp;
		}
	}
}

/*
Reverse values in place of each column in 2D matrix data[rows][cols] or in 1D pointer with logical rows/cols
[A B C]	->	[D E F]
[D E F]		[A B C]
*/
void reverse_cols(matrix_data_t* data, int rows, int cols) {

	int r, c;
	matrix_data_t temp;
	matrix_data_t* pRowA = NULL;
	matrix_data_t* pRowB = NULL;

	for (c = 0; c < cols; ++c) {
		for (r = 0; r < (int)(rows / 2); ++r) { /* explicit truncate */
			pRowA = data + r * cols;
			pRowB = data + cols * (rows - 1 - r);
			temp = pRowA[c];
			pRowA[c] = pRowB[c];
			pRowB[c] = temp;
		}
	}
}

/* Transpose NxM matrix to MxN matrix in O(n) time */
void transpose(const matrix_data_t* src, matrix_data_t* dst, int N, int M) {

	int i;
	for (i = 0; i<N*M; ++i) dst[(i%M)*N + (i / M)] = src[i];	/* one-liner version */

																/*
																expanded version of one-liner:  calculate XY based on array index, then convert that to YX array index
																int i,j,x,y;
																for (i = 0; i < N*M; ++i) {
																x = i % M;
																y = (int)(i / M);
																j = x * N + y;
																dst[j] = src[i];
																}
																*/

																/*
																nested for loop version
																using ptr arithmetic to get proper row/column
																this is really just dst[col][row]=src[row][col]

																int r, c;

																for (r = 0; r < rows; ++r) {
																for (c = 0; c < cols; ++c) {
																(dst + c * rows)[r] = (src + r * cols)[c];
																}
																}
																*/
}

/*
Transpose NxN matrix in place
*/
void transpose_inplace(matrix_data_t* data, int N) {

	int r, c;
	matrix_data_t temp;

	for (r = 0; r < N; ++r) {
		for (c = r; c < N; ++c) { /*start at column=row*/
								  /* using ptr arithmetic to get proper row/column */
								  /* this is really just
								  temp=dst[col][row];
								  dst[col][row]=src[row][col];
								  src[row][col]=temp;
								  */
			temp = (data + c * N)[r];
			(data + c * N)[r] = (data + r * N)[c];
			(data + r * N)[c] = temp;
		}
	}
}

/*
Rotate 1D or 2D src matrix to dst matrix in a direction (90,180,-90)
Precondition:  src and dst are 2d matrices with dimensions src[rows][cols] and dst[cols][rows] or 1D pointers with logical rows/cols
*/
void rotate(int direction, const matrix_data_t* src, matrix_data_t* dst, int rows, int cols) {

	switch (direction) {
	case -90:
		transpose(src, dst, rows, cols);
		reverse_cols(dst, cols, rows);
		break;
	case 90:
		transpose(src, dst, rows, cols);
		reverse_rows(dst, cols, rows);
		break;
	case 180:
	case -180:
		/* bit copy to dst, use in-place reversals */
		memcpy(dst, src, rows*cols * sizeof(matrix_data_t));
		reverse_cols(dst, cols, rows);
		reverse_rows(dst, cols, rows);
		break;
	}
}

/*
Rotate array in a direction.
Array must be NxN 2D or 1D array with logical rows/cols
Direction can be (90,180,-90,-180)
*/
void rotate_inplace(int direction, matrix_data_t* data, int n) {

	switch (direction) {
	case -90:
		transpose_inplace(data, n);
		reverse_cols(data, n, n);
		break;
	case 90:
		transpose_inplace(data, n);
		reverse_rows(data, n, n);
		break;
	case 180:
	case -180:
		reverse_cols(data, n, n);
		reverse_rows(data, n, n);
		break;
	}
}