#include "matrix.h"
#include "kernel.h"

#ifndef CONVOLVE_H
#define CONVOLVE_H

/* 
Convolve 2d matrix by kernel, placing result in 'out'
	Returns flag of success
	Supported types:  'same'
	In-place operations not supported
*/
bool conv2( matrix_t* out, const matrix_t* in, const kernel_t*, const char* type );

#endif
