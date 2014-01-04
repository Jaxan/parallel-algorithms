#include <includes.hpp>
#include <utilities.hpp>

#include "wavelet.hpp"
#include "wavelet2.hpp"


template <typename T>
void print_vec(std::vector<T> v){
	auto it = v.begin(), end = v.end();
	std::cout << "{" << *it++;
	while(it != end) std::cout << ", " << *it++;
	std::cout << "}\n";
}

void timing_test(){
	std::vector<double> input1 = {-1.0, -2.0, 2.0, 1.0, -3.0, -4.0, 4.0, 3.0};
	std::vector<double> input2 = input1;
	int test_size = 10;

	{ timer t("newwvlt");
		auto const n = input1.size();
		for(int i = 0; i < test_size; ++i)
			wvlt::V2::wavelet_mul(input2.data(), input2[0], input2[1], n, 1);
		for(int i = 0; i < test_size; ++i)
			wvlt::V2::wavelet_inv(input2.data(), input2[n-1], input2[n-2], n, 1);
	}
	{ timer t("wavelet");
		for(int i = 0; i < test_size; ++i)
			wvlt::V1::wavelet_mul(input1.begin(), input1.end());
		for(int i = 0; i < test_size; ++i)
			wvlt::V1::wavelet_inv(input1.begin(), input1.end());
	}

	print_vec(input1);
	print_vec(input2);
}

void correctness_test(){
	std::vector<double> input1 = {-1.0, -2.0, 2.0, 1.0, -3.0, -4.0, 4.0, 3.0};
	std::vector<double> input2 = input1;

	wvlt::V1::wavelet(input1.begin(), input1.end());
	wvlt::V1::unwavelet(input1.begin(), input1.end());

	wvlt::V2::wavelet(input2.data(), input2.size());
	wvlt::V2::unwavelet(input2.data(), input2.size());

	std::cout << "V1\t"; print_vec(input1);
	std::cout << "V2\t"; print_vec(input2);
}

int main(){
	std::cout << "*** CORRECTNESS TEST ***\n";
	correctness_test();
	std::cout << "*** TIMING TEST ***\n";
	timing_test();
}

