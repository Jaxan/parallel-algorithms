#pragma once

namespace jcmp {
	// input in [0, 1], output [0, 255]
	inline uint8_t clamp_round(double x){
		if(x <= 0.0) return 0;
		if(x >= 1.0) return 255;
		return uint8_t(std::round(255.0*x));
	}

	// parameters which need to be stored in the file
	// to be able to dequantize
	struct quantize_params {
		double f_max_abs;
		double f_min_abs;
	};

	// Quantization may be non linear (given by f and f_inv)
	struct quantization {
		std::function<double(double)> f;
		std::function<double(double)> f_inv;
		quantize_params p;

		quantization() = default;

		template <typename F>
		quantization(F const & f_, F const & f_inv_, double max_abs, double min_abs)
		: f(f_)
		, f_inv(f_inv_)
		, p{f(max_abs), f(min_abs)}
		{}

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

	static_assert(sizeof(quantize_params) == 16, "struct not propery packed");
}
