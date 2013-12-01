#pragma once

#include <cmath>

namespace wvlt {
	// first row of the matrix Wn
	static double const evn_coef[] = {
		(1.0 + std::sqrt(3.0))/(std::sqrt(32.0)),
		(3.0 + std::sqrt(3.0))/(std::sqrt(32.0)),
		(3.0 - std::sqrt(3.0))/(std::sqrt(32.0)),
		(1.0 - std::sqrt(3.0))/(std::sqrt(32.0))
	};

	// second row of the matrix Wn
	static double const odd_coef[] = {
		 evn_coef[3],
		-evn_coef[2],
		 evn_coef[1],
		-evn_coef[0]
	};

	// first (shifted) row of the matrix Wn^-1
	static double const evn_coef_inv[] = {
		 evn_coef[2],
		 evn_coef[1],
		 evn_coef[0],
		 evn_coef[3]
	};

	// second (shifted) row of the matrix Wn^-1
	static double const odd_coef_inv[] = {
		 evn_coef[3],
		-evn_coef[0],
		 evn_coef[1],
		-evn_coef[2]
	};
}
