#ifndef SAMPLE_H
#define SAMPLE_H

#include "matrix.h"
#include "kernel.h"

/* 
	Downsample a matrix using provided kernel
	Currently supported kernels:  [1 0; 0 0] (2x2 downsample)
*/
bool downsample(matrix_t* out, const matrix_t* in, const kernel_t*);

#endif

