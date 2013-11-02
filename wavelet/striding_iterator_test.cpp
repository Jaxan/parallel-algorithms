#include <iostream>
#include <vector>

#include <boost/assign.hpp>

#include "striding_iterator.hpp"

template <typename Iterator>
striding_iterator<Iterator> double_stride(striding_iterator<Iterator> it){
	it.stride *= 2;
	return it;
}

template <typename Iterator>
void print(Iterator begin, Iterator end){
	while(begin < end){
		std::cout << *begin++ << ", ";
	}
}

template <typename Iterator>
void print_some_rec(Iterator begin, Iterator end){
	print(begin, end);
	std::cout << std::endl;

	if(std::distance(begin, end) >= 2){
		print_some_rec(double_stride(begin), double_stride(end));
		print_some_rec(double_stride(begin+1), double_stride(end));
	}
}

template <typename Iterator>
void print_some(Iterator begin, Iterator end){
	print_some_rec(strided(begin, 1), strided(end, 1));
}

int main(){
	using namespace boost::assign;
	std::vector<int> v;
	v += 0,1,2,3,4;

	print_some(v.begin(), v.end());
}
