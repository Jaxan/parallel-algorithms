#pragma once

static double const evn_coef[] = {
	(1.0 + std::sqrt(3.0))/(std::sqrt(32.0)),
	(3.0 + std::sqrt(3.0))/(std::sqrt(32.0)),
	(3.0 - std::sqrt(3.0))/(std::sqrt(32.0)),
	(1.0 - std::sqrt(3.0))/(std::sqrt(32.0))
};

static double const odd_coef[] = {
	 evn_coef[3],
	-evn_coef[2],
	 evn_coef[1],
	-evn_coef[0]
};

template <typename Iterator>
void wavelet_mul(Iterator begin, Iterator end){
	int mul = end - begin;
	std::vector<double> out(mul, 0.0);
	for(int i = 0; i < mul; i += 2){
		out[i]   = std::inner_product(evn_coef, evn_coef+4, periodic(begin, end) + i, 0.0);
		out[i+1] = std::inner_product(odd_coef, odd_coef+4, periodic(begin, end) + i, 0.0);
	}
	for(int i = 0; i < mul; ++i){
		*begin++ = out[i];
	}
}

template <typename Iterator>
void wavelet_inv(Iterator begin, Iterator end){
	int mul = end - begin;
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
