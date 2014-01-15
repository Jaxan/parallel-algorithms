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
		// The structs proc_info and plan_1D contain some often
		// used values in the parallel algorithm, they also
		// precompute some constants.

		// p = nproc(), s = pid()
		// prev/next = previous and next processor index
		struct proc_info {
			unsigned int p, s, prev, next;

			proc_info(unsigned int p_, unsigned int s_)
			: p(p_), s(s_), prev((s-1+p)%p), next((s+1)%p)
			{}
		};

		// n = inputisze, b = blocksize, m = step_size
		// Cm = communication size
		// TODO: describe other vars
		struct plan_1D {
			unsigned int n, b, m, Cm, small_steps, big_steps, remainder;

			plan_1D(unsigned int n_, unsigned int b_, unsigned int m_)
			: n(n_), b(b_), m(m_), Cm(pow_two(m+1) - 2), small_steps(two_log(b) - 1), big_steps(small_steps/m), remainder(small_steps - m*big_steps)
			{}
		};

		inline plan_1D get_remainder(plan_1D plan){
			plan.m = plan.remainder;
			plan.Cm = pow_two(plan.m+1) - 2;
			plan.remainder = 0;
			return plan;
		}

		inline void comm_step(proc_info const & pi, plan_1D const & plan, double* x, double* other, unsigned int size, unsigned int stride){
			for(unsigned int i = 0; i < plan.Cm; ++i){
				bsp::put(pi.prev, &x[stride*i], other, i, 1);
			}
		}

		inline void comp_step(proc_info const & d, plan_1D const & plan, double* x, double* other, unsigned int size, unsigned int stride){
			unsigned int end = pow_two(plan.m);
			for(unsigned int i = 1; i < end; i <<= 1){
				wavelet_mul(x, other[0], other[i], size, stride*i);
				if(i < end/2) wavelet_mul_base(other, 2*end - 2*i, i);
			}
		}

		inline void step(proc_info const & d, plan_1D const & plan, double* x, double* other, unsigned int size, unsigned int stride){
			comm_step(d, plan, x, other, size, stride);
			bsp::sync();
			comp_step(d, plan, x, other, size, stride);
		}

		inline void base(proc_info const & d, plan_1D const & plan, double* x, double* other, unsigned int size){
			// do steps of size m
			unsigned int stride = 1;
			for(unsigned int i = plan.big_steps; i; i--){
				step(d, plan, x, other, size, stride);
				stride <<= plan.m;
			}

			// in the case m didn't divide the total number of small steps, do the remaining part
			if(plan.remainder)
				step(d, get_remainder(plan), x, other, size, stride);
		}

		// block distributed parallel wavelet, result is also in block distribution (in-place in x)
		inline void wavelet(proc_info const & d, plan_1D const & plan, double* x, double* next, double* proczero){
			// First do the local part
			base(d, plan, x, next, plan.b);

			// Then do a fan in (i.e. 2 elements to proc zero)
			for(unsigned int i = 0; i < 2; ++i){
				bsp::put(0, &x[i * plan.b/2], proczero, d.s * 2 + i);
			}
			bsp::sync();

			// proc zero has the privilige/duty to finish the job
			if(d.s == 0) {
				wvlt::wavelet(proczero, 2*d.p, 1);
				// and to send it back to everyone
				for(unsigned int t = 0; t < d.p; ++t){
					for(unsigned int i = 0; i < 2; ++i){
						bsp::put(t, &proczero[t*2 + i], x, i * plan.b/2);
					}
				}
			}
		}
	}
}

