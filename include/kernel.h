#include <stdlib.h>
#include "matrix.h"

#ifndef KERNEL_H
#define KERNEL_H

typedef matrix_t kernel_t;	/* kernel == matrix */

/* returns the matrix associated with the provided kernel */
static inline matrix_t* kernel_matrix(const kernel_t* k) { return (matrix_t*)k; }

/* intialize kernel to default state */
static inline void kernel_init_default(kernel_t* k) {
	matrix_init_default(kernel_matrix(k));
}

/* Initializes a box kernel with specified size and optional data
Data length must equal size^2 or greater
Data, if specified, will be converted to matrix_data_t
*/
static inline bool kernel_init(kernel_t* k, size_t size, const double* data) {
	if (!k || !size)
		return false;

	if (!matrix_init((matrix_t*)k, size, size))
		return false;

	if (data)
		matrix_assign_d(kernel_matrix(k), data);

	return true;
}

/* initialize a 2x2 downsampling kernel */
static inline bool kernel_init_downsample_2x2(kernel_t* k) {
	/*
	[1 0
	0 0]
	*/
	static const double kernel_data[4] = { 1.,0.,0.,0. };
	return kernel_init(k, 2, kernel_data);
}

/* return size of square kernel_t */
static inline size_t kernel_size(const kernel_t* k) { return matrix_rows(k); }

/*
Initialize fspecial kernel
Supported types:  
	- "average", first parameter:  size.  [ 1/size...; 1/size... ]
	- "prewitt", horizontal prewitt.  [ 1 1 1; 0 0 0; -1 -1 -1 ]
returns flag of success
*/
bool kernel_fspecial( kernel_t*, const char* name, double p1, double p2);


/* destroy internal kernel resources */
static inline void kernel_destroy(kernel_t* k) {
	matrix_destroy(kernel_matrix(k));
}

/* destroys a kernel pointer and frees its internal resources, sets pointer to null */
static inline void kernel_free(kernel_t** pk ) {
	kernel_destroy(*pk);
	free(*pk);
	*pk = NULL;
}

#endif

