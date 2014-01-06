#pragma once

#include <utilities.hpp>
#include "wavelet_constants.hpp"

/* Rewrite of the basic functions
 * This will make the adaption for the parallel case easier,
 * because we can explicitly pass the two elements which are out of range
 * (these are normally wrap-around values)
 *
 * These are also faster (testcase: size = 8, stride = 1, iterations = 100000)
 * V2	0.00377901
 * V1	0.0345114
 *
 * But also less abstract (which can be both a good thing and bad thing)
 *
 * wavelet function does not shuffle!
 */

namespace wvlt{
	inline namespace V2 {
		inline double inner_product(double* x, double const* coef, unsigned int stride){
			return x[0] * coef[0] + x[stride] * coef[1] + x[2*stride] * coef[2] + x[3*stride] * coef[3];
		}

		// will calculate part of wavelete transform (in place!)
		// size is size of vector x (so x[size-1] is valid)
		// does not calculate "last two" elements (it does not assume periodicity)
		// calculates size/stride - 2 elements of the output
		inline void wavelet_mul_base(double* x, unsigned int size, unsigned int stride){
			assert(x && is_even(size) && is_pow_of_two(stride) && 4*stride <= size);

			for(unsigned int i = 0; i < size - 2*stride; i += 2*stride){
				double y1 = inner_product(x + i, evn_coef, stride);
				double y2 = inner_product(x + i, odd_coef, stride);
				x[i] = y1;
				x[i+stride] = y2;
			}
		}

		// x1 and x2 are next elements, or wrap around
		// calculates size/stride elements of the output
		inline void wavelet_mul(double* x, double x1, double x2, unsigned int size, unsigned int stride){
			assert(x && is_even(size) && is_pow_of_two(stride) && 4*stride <= size);
			wavelet_mul_base(x, size, stride);

			unsigned int i = size - 2*stride;
			double y1 = x[i] * evn_coef[0] + x[i+stride] * evn_coef[1] + x1 * evn_coef[2] + x2 * evn_coef[3];
			double y2 = x[i] * odd_coef[0] + x[i+stride] * odd_coef[1] + x1 * odd_coef[2] + x2 * odd_coef[3];
			x[i] = y1;
			x[i+stride] = y2;
		}

		// will overwrite x, x2 and x1 are previous elements, or wrap around
		// size is size of vector x (so x[size-1] is valid)
		inline void wavelet_inv(double* x, double x1, double x2, unsigned int size, unsigned int stride){
			assert(x && is_even(size) && is_pow_of_two(stride) && 4*stride <= size);

			for(unsigned int i = size - 2*stride; i >= 2*stride; i -= 2*stride){
				double y1 = inner_product(x + i - 2*stride, evn_coef_inv, stride);
				double y2 = inner_product(x + i - 2*stride, odd_coef_inv, stride);
				x[i] = y1;
				x[i+stride] = y2;
			}

			unsigned int i = 0;
			double y1 = x2 * evn_coef_inv[0] + x1 * evn_coef_inv[1] + x[i] * evn_coef_inv[2] + x[i+stride] * evn_coef_inv[3];
			double y2 = x2 * odd_coef_inv[0] + x1 * odd_coef_inv[1] + x[i] * odd_coef_inv[2] + x[i+stride] * odd_coef_inv[3];
			x[i] = y1;
			x[i+stride] = y2;
		}

		// size indicates number of elements to process (so this is different from above!)
		inline void wavelet(double* x, unsigned int size, unsigned int stride){
			assert(x && is_pow_of_two(size) && size >= 4);
			auto full_size = stride*size;
			for(unsigned int i = 1; i <= size / 4; i <<= 1){
				auto j = stride * i;
				wavelet_mul(x, x[0], x[j], full_size, j);
			}
		}

		// size indicates number of elements to process (so this is different from above!)
		inline void unwavelet(double* x, unsigned int size, unsigned int stride){
			assert(x && is_pow_of_two(size) && size >= 4);
			auto full_size = stride*size;
			for(unsigned int i = size / 4; i >= 1; i >>= 1){
				auto j = stride * i;
				wavelet_inv(x, x[full_size-j], x[full_size-2*j], full_size, j);
			}
		}
	}
}
