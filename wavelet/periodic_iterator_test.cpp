#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/assign.hpp>

#include "periodic_iterator.hpp"
#include "striding_iterator.hpp"

template <typename Iterator>
void print10(Iterator it){
	for(int i = 0; i < 10; ++i){
		std::cout << *it++ << std::endl;
	}
}

int main(){
	using namespace boost::assign;
	std::vector<int> v;
	v += 0,1,2,3,4;

	print10(periodic(v.begin(), v.end()));
	std::cout << "***\n";
	print10(periodic(strided(v.begin(), 1), strided(v.end(), 1)) + 2);
	std::cout << "***\n";
	std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, "\n"));
}
