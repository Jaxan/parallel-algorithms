#include <vector>
#include <iostream>
#include <iterator>
#include <numeric>
#include <cassert>
#include <cmath>
#include <boost/assign.hpp>

#include "striding_iterator.hpp"
#include "periodic_iterator.hpp"

#include "wavelet.hpp"

bool is_pow_of_two(int n){
	return (n & (n - 1)) == 0;
}

template <typename Iterator>
void shuffle(Iterator begin, Iterator end){
	typedef typename std::iterator_traits<Iterator>::value_type value_type;
	typedef typename std::iterator_traits<Iterator>::difference_type diff_type;
	diff_type s = end - begin;
	assert(s % 2 == 0);

	std::vector<value_type> v(s);
	std::copy(strided(begin  , 2), strided(end  , 2), v.begin());
	std::copy(strided(begin+1, 2), strided(end+1, 2), v.begin() + s/2);
	std::copy(v.begin(), v.end(), begin);
}

template <typename Iterator>
void unshuffle(Iterator begin, Iterator end){
	typedef typename std::iterator_traits<Iterator>::value_type value_type;
	typedef typename std::iterator_traits<Iterator>::difference_type diff_type;
	diff_type s = end - begin;
	assert(s % 2 == 0);

	std::vector<value_type> v(s);
	std::copy(begin, begin + s/2, strided(v.begin(),   2));
	std::copy(begin + s/2, end,   strided(v.begin()+1, 2));
	std::copy(v.begin(), v.end(), begin);
}

template <typename Iterator>
void wavelet(Iterator begin, Iterator end){
	int s = end - begin;
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
	int s = end - begin;
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

struct filter{
	filter(double threshold)
	: threshold(threshold)
	{}

	void operator()(double& x){
		if(std::abs(x) <= threshold) x = 0;
	}

	double threshold;
};

int main(){
	using namespace boost::assign;
	std::vector<double> input;
	input += 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;

	// print input
	std::copy(input.begin(), input.end(), std::ostream_iterator<double>(std::cout, "\n"));
	std::cout << std::endl;

	std::vector<double> thresholds;
	thresholds += 0.0, 0.1, 0.2, 0.5;
	for(int i = 0; i < thresholds.size(); ++i){
		std::vector<double> v;
		v = input;

		// transform to wavelet domain
		wavelet(v.begin(), v.end());

		// apply threshold
		std::for_each(v.begin(), v.end(), filter(thresholds[i]));
		int zeros = std::count(v.begin(), v.end(), 0.0);

		// transform back to sample domain
		unwavelet(v.begin(), v.end());

		// print compressed
		std::cout << "\ncp: " << zeros / double(v.size()) << std::endl;
		std::copy(v.begin(), v.end(), std::ostream_iterator<double>(std::cout, "\n"));
		std::cout << std::endl;
	}
}
