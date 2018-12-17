#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

#ifndef MATRIX_H
#define MATRIX_H

typedef float matrix_data_t;
typedef struct Matrix matrix_t;

struct Matrix {
	size_t rows;
	size_t cols;
	matrix_data_t* data;
	const matrix_t* parent;
	size_t start_col;
};

/* matrix initialization to default state */
static inline void matrix_init_default(matrix_t*m) {
	m->rows = 0;
	m->cols = 0;
	m->data = NULL;
	m->parent = NULL;
	m->start_col = 0;
}

/* matrix initialization with rows, cols, data; returns flag of success */
bool matrix_init( matrix_t*, const size_t rows, const size_t cols );

/* matrix malloc with dimensions, returns NULL on failure */
matrix_t* matrix_malloc(const size_t rows, const size_t cols);

/* destroys existing matrix if any, initializes to specified rows/cols */
bool matrix_reset(matrix_t*, const size_t rows, const size_t cols);

/* destroys a matrix internal resources */
void matrix_destroy(matrix_t*);

/* swap the contents of two matrices */
void matrix_swap(matrix_t*, matrix_t*);

/* destroys matrix internal data and frees the pointer, sets pointer to null */
static inline void matrix_free(matrix_t** p) {
	if (!p || !*p)
		return;
	matrix_destroy(*p);
	free(*p);
	*p = NULL;
}

/* copies matrix_data to pointer */
void matrix_copy_data(matrix_data_t* out, const matrix_t* in);

/* deep copies a matrix and adds elements of padding to each X and Y edges.  return flag if successful */
bool matrix_pad( matrix_t* out, const matrix_t*, const size_t padX, const size_t padY );

/* crop the provided matrix to size cols*rows, starting at specified row & column of input matrix.  return flag if successful */
bool matrix_crop(matrix_t* out, const matrix_t*, const size_t rows, const size_t cols, const size_t start_col_index, const size_t start_row_index );

/*computes number of elements in a matrix (dim1*dim2*dim3, where dimX is ignored if zero) */
static inline size_t matrix_size(const matrix_t* m) { return m->cols * m->rows; }

/*returns number of rows in a 2d matrix*/
static inline size_t matrix_rows(const matrix_t* m) { return m->rows; }

/*returns number of columns in a 1d or 2d matrix*/
static inline size_t matrix_cols(const matrix_t* m) { return m->cols; }

/* returns mutable pointer to the start of the data in matrix */
static inline matrix_data_t* matrix_data( matrix_t* m ) { return m->data; }

/* returns const pointer to the start of the data in matrix */
static inline const matrix_data_t* matrix_cdata( const matrix_t* m ) { return m->data; }

/* resets matrix data to zero */
static inline void matrix_clear(matrix_t* m) {
	if (m)
		memset(m->data, 0, matrix_size(m) * sizeof(matrix_data_t));
}

/* return mutable pointer to matrix data at row index.  Provides fast access, not bounds checked */
static inline matrix_data_t* matrix_row( matrix_t* m, const size_t row) {
	return matrix_data(m) + m->start_col
		+ matrix_cols( m->parent ? m->parent : m) * row
		;
}

/* return const pointer to matrix data at row index.  Provides fast access, not bounds checked */
static inline const matrix_data_t* matrix_crow(const matrix_t* m, const size_t row) {
	return matrix_cdata(m) + m->start_col
		+ matrix_cols(m->parent ? m->parent : m) * row
		;
}

/* creates a non-owning submatrix of provided matrix, starting at specified row.  if col != 0, returned matrix will be a non-contiguous matrix */
void matrix_submatrix(matrix_t* out, const matrix_t* in, const size_t rows, const size_t cols, const size_t start_row, const size_t start_col);

/* returns flag if matrix is a submatrix of another matrix */
static inline bool matrix_is_submatrix(const matrix_t* m) { return m->parent; }

/* returns flag if matrix is contiguous in memory */
static inline bool matrix_is_contiguous(const matrix_t* m) {
	if (!m->parent)
		return true;
	return
		m->start_col == 0
		&& matrix_cols(m->parent) == matrix_cols(m)
		;
}

/* rotate a matrix by 90,-90,180 degrees, returns flag of success */
bool matrix_rotate(matrix_t* out, const matrix_t*, int direction);

/* rotate a matrix in place by 90,-90,180 degrees.  precondition:  matrix rows==cols */
void matrix_rotate_inplace( matrix_t*, int direction);

/* transpose a matrix, returning flag of success.  In-place operation not supported */
bool matrix_transpose(matrix_t*, const matrix_t*);

/* Copies double data to matrix, data is converted to matrix_data_t */
void matrix_assign_d( matrix_t*, const double* );

/* Copies matrix_data_t to matrix */
void matrix_assign(matrix_t*, const matrix_data_t*);

/* Copies double data to matrix, data is converted to matrix_data_t */
void matrix_assign_u8(matrix_t*, const uint8_t* );

/* coefficient-wise multiply two matrices
	in-place operation supported
*/
void matrix_cwise_scale(matrix_t* out, const matrix_t*, const matrix_t*);

/* element-wise scale (multiply) two matrices, then sum the result */
double matrix_cwise_scale_sum(const matrix_t*, const matrix_t* );

/* coefficient-wise scale matrix by scalar value
	in-place operation supported
*/
void matrix_cwise_scale_scalar(matrix_t* out, const matrix_t*, const matrix_data_t );

/* 
coefficient-wise divide matrix by another matrix
in-place operation supported
*/
void matrix_cwise_divide(matrix_t* out, const matrix_t*, const matrix_t*);

/* coefficient-wise square
	in-place operation supported
	*/
void matrix_cwise_square(matrix_t* out, const matrix_t* );

/* coefficient-wise calculate square root; in-place operation supported */
void matrix_cwise_sqrt( matrix_t* out, const matrix_t* );

/* coefficient-wise add two matrices
in-place operation supported
*/
void matrix_cwise_add(matrix_t* out, const matrix_t*, const matrix_t*);

/* coefficient-wise add a matrix and a scalar
in-place operation supported
*/
void matrix_cwise_add_scalar(matrix_t* out, const matrix_t*, const matrix_data_t scalar );

/* Computes sum of all elements in the matrix */
double matrix_sum(const matrix_t*);

/* Computes the population variance of all elements in the matrix */
double matrix_variance_p(const matrix_t*);

/* Computes the population standard deviation of all elements in the matrix */
double matrix_stdev_p(const matrix_t*);

#endif 

