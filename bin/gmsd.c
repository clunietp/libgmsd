#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

#ifdef _WIN32
#include <crtdbg.h>
#endif

#include <libgmsd.h>
#include <bmpread.h>

/*
	convert data in bmpread struct to grayscale
	0.299 * R + 0.587 * G + 0.114 * B
*/
gmsd_image_data_t* rgb2gray(const bmpread_t* rgb) {

	gmsd_image_data_t* result = NULL;
	unsigned char* pSrc = NULL;
	gmsd_image_data_t* pDst = NULL;
	size_t i = 0;
	double val = 0.;
	
	if (!rgb || !rgb->width || !rgb->height || !rgb->data) {
		return NULL;
	}
	
	pSrc = rgb->data;
	result = malloc(rgb->width*rgb->height * sizeof(gmsd_image_data_t));

	if (!result)
		return NULL;

	pDst = result;

	for (i = 0; i < (size_t)(rgb->width*rgb->height); ++i) {
		val = .299*(double)*pSrc++;
		val += .587*(double)*pSrc++;
		val += .114*(double)*pSrc++;
		*pDst++ = (gmsd_image_data_t) round( val );
	};

	return result;
}

bool compare(const char* ref_path, const char* cmp_path, double* val ) {
	
	bmpread_t
		ref = { 0 }
		, cmp = { 0 }
	;

	gmsd_image_data_t
		*ref_gs = NULL
		, *cmp_gs = NULL
		;
	
	bool result = false;

	/* attempt read of ref, cmp; read data top-down iaw matlab */
	if (
		!val
		|| !bmpread(ref_path, BMPREAD_ANY_SIZE | BMPREAD_BYTE_ALIGN | BMPREAD_TOP_DOWN, &ref)
		|| !bmpread(cmp_path, BMPREAD_ANY_SIZE | BMPREAD_BYTE_ALIGN | BMPREAD_TOP_DOWN, &cmp)
		)
		goto CLEANUP;
	
	/* convert to grayscale */
	ref_gs = rgb2gray(&ref);
	cmp_gs = rgb2gray(&cmp);

	if (!ref_gs || !cmp_gs)
		goto CLEANUP;

	result = gmsd(ref_gs, cmp_gs, (size_t)ref.width, (size_t)ref.height, val, NULL);

CLEANUP:
	bmpread_free(&ref);
	bmpread_free(&cmp);
	free(ref_gs);
	free(cmp_gs);

	return result;
}

int main( int argc, char* argv[] )
{
	int result = (argc==3);
	double val=0.;

	if (!result) {
		printf("GMSD - Gradient Magnitude Similarity Deviation\n");
		printf("Version 1.0\n");
		printf("Copyright(C) 2018 Tom Clunie\n");
		printf("https://github.com/clunietp/libgmsd\n\n");
		printf("Implementation of GMSD algorithm by Xue, Zhang, Mou and Bovik\nhttp://www4.comp.polyu.edu.hk/~cslzhang/IQA/GMSD/GMSD.htm\n\n");
		printf("Usage:  <gmsd> \"path/to/original.bmp\" \"path/to/comparison.bmp\"\n");
		printf("Output:  result of GMSD\n");
		printf("Expected input format:  24BPP BMP\n");
	}
	else {
		if (compare(argv[1], argv[2], &val))
			printf("%f\n", val);
		else {
			printf("INPUT ERROR");
			result = -1;
		}
			
	}

    return result;
}

