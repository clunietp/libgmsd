#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>	
#include <sys/timeb.h>
#include <assert.h>

#ifdef _WIN32
#include <crtdbg.h>
#endif

#include <libgmsd.h>
#include <bmpread.h>

double round_to( double a, const unsigned ndigits )
{
	const double
		pow10 = pow(10., (double)ndigits)
		, var = a * pow10;
	return 
		(var - (int)var >= 0.5 ? (int)var + (a > 0. ? 1 : -1) : (int)var)
		/ pow10
		;
}

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
		*pDst++ = (gmsd_image_data_t)round(val);
	};

	return result;
}

/* compare two files via gmsd.  returns true on success,false on failure */
bool compare(const char* ref_path, const char* cmp_path, double expected, size_t n_iters ) {
	
	bmpread_t
		ref = { 0 }
		, cmp = { 0 }
	;
	gmsd_image_data_t
		*ref_gs = NULL
		, *cmp_gs = NULL
		, *qm = NULL
		;
	
	double gmsd_result = 0.;
	size_t i = 0;
	bool result = false;

	/* attempt read of ref, cmp; read data top-down iaw matlab */
	if (
		!bmpread(ref_path, BMPREAD_ANY_SIZE | BMPREAD_BYTE_ALIGN | BMPREAD_TOP_DOWN, &ref)
		|| !bmpread(cmp_path, BMPREAD_ANY_SIZE | BMPREAD_BYTE_ALIGN | BMPREAD_TOP_DOWN, &cmp)
		)
	{
		printf("Error opening test files:, \"%s\", \"%s\"\n", ref_path, cmp_path);
		goto CLEANUP;
	}

	/* malloc space for quality map output */
	qm = malloc(sizeof(gmsd_image_data_t)*(ref.width / 2)*(ref.height / 2));

	/* convert to grayscale */
	ref_gs = rgb2gray(&ref);
	cmp_gs = rgb2gray(&cmp);

	if (!ref_gs || !cmp_gs || !qm )
		goto CLEANUP;

	/* exec gmsd, n iters */
	for (i=0;i<n_iters;++i)
		gmsd(ref_gs, cmp_gs, (size_t)ref.width, (size_t)ref.height, &gmsd_result, &qm);

	assert(qm != NULL);
	result = round_to(gmsd_result, 4) == round_to(expected, 4);

CLEANUP:
	bmpread_free(&ref);
	bmpread_free(&cmp);
	free(ref_gs);
	free(cmp_gs);
	free(qm);
	return result;
}

int main()
{
#define TEST_FILE_LENGTH 5

	static const char* REF_FILE = "i23.bmp";

	static const char* TEST_FILES[TEST_FILE_LENGTH] = {
		"i23_10_1.bmp"
		, "i23_10_2.bmp"
		, "i23_10_3.bmp"
		, "i23_10_4.bmp"
		, "i23_10_5.bmp"
	};

	static const double TEST_FILES_RESULTS[TEST_FILE_LENGTH] = {
		.0029
		, .0071
		, .0268
		, .1036
		, .1897
	};

	const unsigned BENCHMARK_RUNS = 100;
	int result = 0;
	struct timeb start = { 0 }, stop = { 0 };
	long diff_ms = 0;
	unsigned i = 0;

#ifdef _WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	printf("Running tests....");

	/* 
	comparison values based on original gmsd.m and TID2013 dataset
	http://www4.comp.polyu.edu.hk/~cslzhang/IQA/GMSD/GMSD.htm
	*/

	for (i = 0; i < TEST_FILE_LENGTH; ++i)
		result = compare(REF_FILE, TEST_FILES[i], TEST_FILES_RESULTS[i], 1) 
			? result 
			: -1
		;

	if (result == 0)
		printf("PASSED\n");
	else
		printf("FAILURES DETECTED\n");

/* benchmark */
#ifndef _DEBUG

	printf("Running benchmarks, %u iterations...", BENCHMARK_RUNS);
	ftime(&start);
	compare(REF_FILE, TEST_FILES[0], TEST_FILES_RESULTS[0], BENCHMARK_RUNS);
	ftime(&stop);
	diff_ms = (long)(stop.time - start.time) * 1000 + stop.millitm - start.millitm;

	printf("complete.  Total %u ms, avg %f ms per run\n",
		diff_ms, diff_ms / (double)BENCHMARK_RUNS
	);
#else
	printf("Benchmarks not run in debug mode.  Switch to release mode to run benchmarks\n");
#endif

    return result;
}

