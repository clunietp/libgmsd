#ifndef LIBGMSD_H
#define LIBGMSD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef float gmsd_image_data_t;

/* 
Executes the GMSD algorithm
	ref:			grayscale pixels of original image, range [0-255]
	cmp:			grayscale pixels of comparison image, range [0-255]
	width:			image width in pixels
	height:			image height in pixels
	result:			pointer to double which receives result of GMSD
	pOutMapPx:		(Optional) pointer to pointer of gmsd_image_data_t which receives the resulting quality map
					The dimensions of the output image will be floor(width/2)*floor(height/2)
					This memory must have already been allocated by the caller with size at least floor(width/2)*floor(height/2)*sizeof(gmsd_image_data_t)
Returns 0 on failure, non-zero on success
*/
int gmsd( 
	const gmsd_image_data_t* ref
	, const gmsd_image_data_t* cmp
	, const size_t width
	, const size_t height
	, double* result 
	, gmsd_image_data_t** ppOutMapPx
);

#ifdef __cplusplus
}
#endif
#endif