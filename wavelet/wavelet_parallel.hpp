#pragma once

#include <includes.hpp>
#include <utilities.hpp>
#include <bsp.hpp>
#include "wavelet.hpp"

/* In the following function we assume any in-parameter to be already
 * bsp::pushed. And the functions won't do any bsp::sync at the end. Both
 * conventions make it possible to chains functions with lesser syncs.
 *
 * Distribution is block distribution.
 */

namespace wvlt {
	namespace par {
		// Convenience container of some often-used values
		// n = inputisze, p = nproc(), s = pid()
		// b = blocksize, prev/next = previous and next processor index
		struct distribution {
			unsigned int n, p, s, b, prev, next;

			distribution(unsigned int n_, unsigned int p_, unsigned int s_)
			: n(n_), p(p_), s(s_), b(n/p), prev((s-1+p)%p), next((s+1)%p)
			{}
		};

		inline unsigned int communication_size(unsigned int m){
			return pow_two(m+1) - 2;
		}

		inline void step(distribution const & d, double* x, double* other, unsigned int size, unsigned int stride, unsigned int m){
			unsigned int t = d.prev;
			unsigned int Cm = communication_size(m);
			for(unsigned int i = 0; i < Cm; ++i){
				bsp::put(t, &x[stride*i], other, i, 1);
			}
			bsp::sync();

			unsigned int end = pow_two(m);
			for(unsigned int i = 1; i < end; i <<= 1){
				wavelet_mul(x, other[0], other[i], size, stride*i);
				if(i < end/2) wavelet_mul_base(other, 2*end - 2*i, i);
			}
		}

		inline void base(distribution const & d, double* x, double* other, unsigned int size, unsigned int m){
			unsigned int t = two_log(d.b);
			unsigned int steps = (t-1)/m;

			unsigned int stride = 1;
			for(unsigned int i = steps; i; i--){
				step(d, x, other, size, stride, m);
				stride <<= m;
			}

			unsigned int remaining = (t-1) - m*steps;
			if(remaining)
				step(d, x, other, size, stride, remaining);
		}

		// block distributed parallel wavelet, result is also in block distribution (in-place in x)
		inline void wavelet(distribution const & d, double* x, double* next, double* proczero, unsigned int m){
			// First do the local part
			base(d, x, next, d.b, m);

			// Then do a fan in (i.e. 2 elements to proc zero)
			for(unsigned int i = 0; i < 2; ++i){
				bsp::put(0, &x[i * d.b/2], proczero, d.s * 2 + i);
			}
			bsp::sync();

			// proc zero has the privilige/duty to finish the job
			if(d.s == 0) {
				wvlt::wavelet(proczero, 2*d.p, 1);
				// and to send it back to everyone
				for(unsigned int t = 0; t < d.p; ++t){
					for(unsigned int i = 0; i < 2; ++i){
						bsp::put(t, &proczero[t*2 + i], x, i * d.b/2);
					}
				}
			}
		}
	}
}

