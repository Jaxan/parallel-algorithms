#pragma once

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

		// will overwrite x, x1 and x2 are next elements, or wrap around
		// size is size of vector x (so x[size-1] is valid)
		inline void wavelet_mul(double* x, double x1, double x2, unsigned int size, unsigned int stride){
			assert(is_pow_of_two(size) && is_pow_of_two(stride) && 4*stride <= size);

			for(int i = 0; i < size - 2*stride; i += 2*stride){
				double y1 = inner_product(x + i, evn_coef, stride);
				double y2 = inner_product(x + i, odd_coef, stride);
				x[i] = y1;
				x[i+stride] = y2;
			}

			int i = size - 2*stride;
			double y1 = x[i] * evn_coef[0] + x[i+stride] * evn_coef[1] + x1 * evn_coef[2] + x2 * evn_coef[3];
			double y2 = x[i] * odd_coef[0] + x[i+stride] * odd_coef[1] + x1 * odd_coef[2] + x2 * odd_coef[3];
			x[i] = y1;
			x[i+stride] = y2;
		}

		// will overwrite x, x2 and x1 are previous elements, or wrap around
		// size is size of vector x (so x[size-1] is valid)
		inline void wavelet_inv(double* x, double x1, double x2, unsigned int size, unsigned int stride){
			assert(is_pow_of_two(size) && is_pow_of_two(stride) && 4*stride <= size);

			for(int i = size - 2*stride; i >= 2*stride; i -= 2*stride){
				double y1 = inner_product(x + i - 2*stride, evn_coef_inv, stride);
				double y2 = inner_product(x + i - 2*stride, odd_coef_inv, stride);
				x[i] = y1;
				x[i+stride] = y2;
			}

			int i = 0;
			double y1 = x2 * evn_coef_inv[0] + x1 * evn_coef_inv[1] + x[i] * evn_coef_inv[2] + x[i+stride] * evn_coef_inv[3];
			double y2 = x2 * odd_coef_inv[0] + x1 * odd_coef_inv[1] + x[i] * odd_coef_inv[2] + x[i+stride] * odd_coef_inv[3];
			x[i] = y1;
			x[i+stride] = y2;
		}

		inline void wavelet(double* x, unsigned int size){
			assert(is_pow_of_two(size) && size >= 4);
			for(int i = 1; i <= size / 4; i <<= 1){
				wavelet_mul(x, x[0], x[i], size, i);
			}
		}

		inline void unwavelet(double* x, unsigned int size){
			assert(is_pow_of_two(size) && size >= 4);
			for(int i = size / 4; i >= 1; i >>= 1){
				wavelet_inv(x, x[size-i], x[size-2*i], size, i);
			}
		}
	}
}
