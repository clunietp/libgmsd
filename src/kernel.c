#include "kernel.h"
#include <string.h>

bool fspecial_average( kernel_t* k, size_t size) {

	size_t i;
	matrix_data_t* pData = NULL;

	if ( !k || !size)
		return false;

	kernel_destroy(k);
	kernel_init(k, size, NULL);
	pData = matrix_data(kernel_matrix(k));
	
	for (i = 0; i < (size * 2); ++i)
		*pData++ = (matrix_data_t)(1.0 / (double)(size * 2));

	return true;
}

bool fspecial_prewitt(kernel_t* k ) {
	
	static const double data[9] = { 1., 1.,1.,0.,0.,0.,-1.,-1.,-1. };
	if ( !k )
		return false;
	kernel_destroy(k);
	return kernel_init(k, 3, data);

}

bool kernel_fspecial( kernel_t* k, const char* name, double p1, double p2) {

	if (strcmp(name, "average") == 0)
		return fspecial_average(k, (size_t)p1);
	else if (strcmp(name, "prewitt") == 0)
		return fspecial_prewitt(k);
	return false;
}