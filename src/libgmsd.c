#include "libgmsd.h"
#include "kernel.h"
#include "convolve.h"
#include "sample.h"

#include <stdlib.h>
#include <assert.h>

bool create_quality_map( matrix_t* out, matrix_t* gm1, matrix_t* gm2) {
	/*
	GMS map:
		( 2*gm1*gm2+c ) / ( gm1^2 + gm2^2 + c )
	*/

	static const matrix_data_t C = 170;	/* the paper says 0.0026 but gmsd.m has 170 */

	matrix_t 
		gm1_sq = { 0 }
		, gm2_sq = { 0 }
	;

	matrix_init(&gm1_sq, matrix_rows(gm1), matrix_cols(gm1));
	matrix_init(&gm2_sq, matrix_rows(gm2), matrix_cols(gm2));

	/* denominator */
	matrix_cwise_square(&gm1_sq, gm1);	/* gm1_sq = gm1^2 */
	matrix_cwise_square(&gm2_sq, gm2);	/* gm2_sq = gm2^2 */
	matrix_cwise_add(&gm1_sq, &gm1_sq, &gm2_sq);	/* gm1_sq += gm2_sq */
	matrix_cwise_add_scalar(&gm1_sq, &gm1_sq, C);	/* gm1_sq += T */

	/* numerator */
	matrix_cwise_scale_scalar(gm1, gm1, (matrix_data_t)2);	/* gm1 *= 2 */
	matrix_cwise_scale(gm1, gm1, gm2);	/* gm1 *= gm2 */
	matrix_cwise_add_scalar(gm1, gm1, C);	/* gm1 += T */

	/* gm1 / gm1_sq */
	matrix_cwise_divide(out, gm1, &gm1_sq);

	matrix_destroy(&gm1_sq);
	matrix_destroy(&gm2_sq);

	return true;
}

bool avg_downsample(matrix_t* out, const matrix_t* m ) {

	kernel_t
		k_avg = { 0 }
		, k_downsample = { 0 }
	;
	matrix_t avg = { 0 };
	bool result = false;

	/* init kernels */
	if (
		!kernel_fspecial(&k_avg, "average", 2., 0.)
		|| !kernel_init_downsample_2x2(&k_downsample)
		)
		goto CLEANUP;

	/* apply average kernel */
	if (!conv2(&avg, m, &k_avg, "same"))
		goto CLEANUP;
	
	/* apply downsample kernel */
	if (!downsample(out, &avg, &k_downsample))
		goto CLEANUP;

	result = true;

CLEANUP:
	kernel_destroy(&k_avg);
	kernel_destroy(&k_downsample);
	matrix_destroy(&avg);

	return result;
}

bool create_gradient_map( matrix_t* out, const gmsd_image_data_t* px, const size_t width, const size_t height) {

	kernel_t 
		k_prewitt_x = { 0 }
		, k_prewitt_y = { 0 }
	;

	matrix_t
		m = { 0 }		/* full size ref image matrix */
		, dm = { 0 }	/* downsized ref image matrix */
		, iy = { 0 }	/* y kernel applied */
	;

	bool result = false;

	/* input check */
	if (!px || !width || !height || !out)
		goto CLEANUP;

	/* pixels to matrix */
	if (!matrix_init(&m, height, width) )
		goto CLEANUP;
	matrix_assign(&m, px);

	/* apply 2x2 average filter, then downsample by factor of 2 */
	if ( !avg_downsample(&dm, &m) )
		goto CLEANUP;

	/* done with m */
	matrix_destroy(&m);

	/* create prewitt kernels */
	if (!kernel_fspecial(&k_prewitt_x, "prewitt", 0., 0.))
		goto CLEANUP;

	matrix_cwise_scale_scalar(&k_prewitt_x, &k_prewitt_x, (matrix_data_t)(1. / 3.));/* scale by 1/3 */

	/* transpose x prewitt to y */
	if (!matrix_transpose(&k_prewitt_y, &k_prewitt_x))
		goto CLEANUP;

	/* apply Prewitt kernels */
	/* using 'out' matrix for ix and further in-place processing */
	if (
		!conv2(out, &dm, &k_prewitt_x, "same" )	/* I(x)hx; horizontal gradient img */
		|| !conv2(&iy, &dm, &k_prewitt_y, "same" )	/* I(x)hy; vertical gradient img */
		)
		goto CLEANUP;

	/* create gradient map */
	matrix_cwise_square(out, out);		/* (I(x)hx)^2 */
	matrix_cwise_square(&iy,&iy);		/* (I(x)hy)^2 */
	matrix_cwise_add(out, out, &iy);	/* (I(x)hx)^2 + (I(x)hy)^2 */
	matrix_cwise_sqrt(out, out);		/* sqrt( (I(x)hx)^2 + (I(x)hy)^2 ) */

	result = true;

CLEANUP:

	matrix_destroy(&m);
	matrix_destroy(&dm);
	matrix_destroy(&iy);
	kernel_destroy(&k_prewitt_x);
	kernel_destroy(&k_prewitt_y);
	return result;
}

int gmsd(const gmsd_image_data_t* ref, const gmsd_image_data_t* cmp, const size_t width, const size_t height, double* val, gmsd_image_data_t** ppOutMapPx) {

	matrix_t 
		gm_ref = { 0 }
		, gm_cmp = { 0 }
		, qm = { 0 }
	;
	
	int result = 0;

	if (
		!val
		|| !create_gradient_map(&gm_ref, ref, width, height)
		|| !create_gradient_map(&gm_cmp, cmp, width, height)
		|| !create_quality_map( &qm, &gm_ref, &gm_cmp )
		)
		goto CLEANUP;

	*val = matrix_stdev_p(&qm);

	if (ppOutMapPx) /* caller wants output quality map */
		matrix_copy_data(*ppOutMapPx, &qm);

	result = 1;
CLEANUP:
	matrix_destroy(&gm_ref);
	matrix_destroy(&gm_cmp);
	matrix_destroy(&qm);
	return result;
}