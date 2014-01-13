#pragma once

#include <functional>

namespace jcmp {
	// Will quantize a double such that it fits in a uint8_t
	// Also support non-linear quantization (which is usefull
	// when the data is non-uniformly distributed)
	namespace quantization {
		// input in [0, 1], output [0, 255]
		inline uint8_t clamp_round(double x){
			if(x <= 0.0) return 0;
			if(x >= 1.0) return 255;
			return uint8_t(std::round(255.0*x));
		}

		// parameters which need to be stored in the file
		// to be able to dequantize
		// TODO: add type of quantization
		struct parameters {
			double f_max_abs;
			double f_min_abs;
		};

		// the basic struct doing the quantization
		struct base {
			// f needs to be of type R+ -> R
			// and F-inv should be its inverse
			std::function<double(double)> f;
			std::function<double(double)> f_inv;
			parameters p;

			uint8_t forwards(double x){
				double y = std::abs(x);
				y = (f(y) - p.f_min_abs) / (p.f_max_abs - p.f_min_abs);
				if(x < 0) y = -y;
				return clamp_round(0.5 * (y + 1.0));
			}

			double backwards(uint8_t x){
				double y = 2.0 * x / 255.0 - 1.0;
				y = (p.f_max_abs - p.f_min_abs) * y;
				if(y < 0) return -f_inv(-y + p.f_min_abs);
				return f_inv(y + p.f_min_abs);
			}
		};

		// kinds of quantization which come out of the
		// box with get(...)
		enum class type {
			linear,
			logarithmic,
			square_root
		};

		// non-overloaded versions
		inline double id(double x) { return x; }
		inline double log(double x) { return std::log(x); }
		inline double exp(double x) { return std::exp(x); }
		inline double sqrt(double x) { return std::sqrt(x); }
		inline double sqr(double x) { return x*x; }

		base get(type t, double max_abs, double min_abs, bool apply = true){
			base b;
			switch (t) {
				case type::linear:
					b.f = &id;
					b.f_inv = &id;
					break;
				case type::logarithmic:
					b.f = &log;
					b.f_inv = &exp;
					break;
				case type::square_root:
					b.f = &sqrt;
					b.f_inv = &sqr;
					break;
			}
			if(apply){
				b.p.f_max_abs = b.f(max_abs);
				b.p.f_min_abs = b.f(min_abs);
			} else {
				b.p.f_max_abs = max_abs;
				b.p.f_min_abs = min_abs;
			}
			return b;
		}

		static_assert(sizeof(parameters) == 16, "struct not propery packed");
	}
}
