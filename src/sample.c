#include "sample.h"
#include <stdlib.h>
#include <assert.h>

bool downsample_2x2(matrix_t* out, const matrix_t* in, const kernel_t* k ) {

	const size_t ksize = kernel_size(k);
	size_t r
		, c
		, result_row
		;
	const matrix_data_t *pIn;
	matrix_data_t* pOut;
	
	if (!matrix_reset(out, matrix_rows(in) / 2, matrix_cols(in) / 2))
		return false;

	for (r = 0, result_row=0; r < matrix_rows(in); r += ksize, ++result_row) {

		pIn = matrix_crow(in, r);
		pOut = matrix_row(out, result_row);

		for (c = 0; c < matrix_cols(in); c += ksize, ++pOut, pIn += ksize)
			*pOut = *pIn;
	}

	return true;
}

bool downsample(matrix_t* out, const matrix_t* m, const kernel_t* k) {
	
	/* check for 2x2 kernel */
	if (kernel_size(k) == 2
		&& (*matrix_crow(k,0) == 1.)
		&& (*( matrix_crow(k, 0) + 1) == 0.)
		&& (*matrix_crow(k, 1) == 0.)
		&& (*(matrix_crow(k, 1) + 1) == 0.)
		)
		return downsample_2x2(out, m, k);

	assert(false);/*not implemented*/
	return false;
}