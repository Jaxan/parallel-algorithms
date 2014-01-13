#pragma once

#include <includes.hpp>

#include "striding_iterator.hpp"
#include "periodic_iterator.hpp"
#include "wavelet_constants.hpp"

/* This header is deprecated. Use wavelet_2.hpp instead. It's still here
 * for checking correctness of implementations, as this one is correct, but
 * very naive (and hence slow).
 */

namespace wvlt {
	namespace V1 {
		// Apply the matrix Wn with the DAUB4 coefficients
		template <typename Iterator>
		void wavelet_mul(Iterator begin, Iterator end){
			auto mul = end - begin;
			std::vector<double> out(mul, 0.0);
			for(int i = 0; i < mul; i += 2){
				out[i]   = std::inner_product(evn_coef, evn_coef+4, periodic(begin, end) + i, 0.0);
				out[i+1] = std::inner_product(odd_coef, odd_coef+4, periodic(begin, end) + i, 0.0);
			}
			for(int i = 0; i < mul; ++i){
				*begin++ = out[i];
			}
		}

		// Apply inverse of the matrix Wn with the DAUB4 coefficients
		template <typename Iterator>
		void wavelet_inv(Iterator begin, Iterator end){
			auto mul = end - begin;
			std::vector<double> out(mul, 0.0);
			Iterator bc = begin;
			for(int i = 0; i < mul; i += 2, begin += 2){
				Iterator b2 = begin + 1;
				for(int j = 0; j < 4; ++j){
					out[(i+j) % mul] += *begin * evn_coef[j] + *b2 * odd_coef[j];
				}
			}
			for(int i = 0; i < mul; ++i){
				*bc++ = out[i];
			}
		}

		// Shuffle works with an extra copy, might be inefficient, but it is at least correct ;)
		// It applies the even-odd-sort matrix Sn
		template <typename Iterator>
		void shuffle(Iterator begin, Iterator end){
			typedef typename std::iterator_traits<Iterator>::value_type value_type;
			auto s = end - begin;
			assert(s % 2 == 0);

			std::vector<value_type> v(s);
			std::copy(strided(begin  , 2), strided(end  , 2), v.begin());
			std::copy(strided(begin+1, 2), strided(end+1, 2), v.begin() + s/2);
			std::copy(v.begin(), v.end(), begin);
		}

		template <typename Iterator>
		void unshuffle(Iterator begin, Iterator end){
			typedef typename std::iterator_traits<Iterator>::value_type value_type;
			auto s = end - begin;
			assert(s % 2 == 0);

			std::vector<value_type> v(s);
			std::copy(begin, begin + s/2, strided(v.begin(),   2));
			std::copy(begin + s/2, end,   strided(v.begin()+1, 2));
			std::copy(v.begin(), v.end(), begin);
		}

		// Combines the matrix Wn and Sn recusrively
		// Only works for inputs of size 2^m
		template <typename Iterator>
		void wavelet(Iterator begin, Iterator end){
			auto s = end - begin;
			assert(s >= 4);
			for(int i = s; i >= 4; i >>= 1){
				// half interval
				end = begin + i;
				assert(is_pow_of_two(end - begin));

				// multiply with Wn
				wavelet_mul(begin, end);
				// then with Sn
				shuffle(begin, end);
			}
		}

		template <typename Iterator>
		void unwavelet(Iterator begin, Iterator end){
			auto s = end - begin;
			assert(s >= 4);
			for(int i = 4; i <= s; i <<= 1){
				// double interval
				end = begin + i;
				assert(is_pow_of_two(end - begin));

				// unshuffle: Sn^-1
				unshuffle(begin, end);
				// then Wn^-1
				wavelet_inv(begin, end);
			}
		}
	}
}
