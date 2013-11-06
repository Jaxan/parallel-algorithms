#pragma once

#include <string>

inline bool is_pow_of_two(int n){
	return (n & (n - 1)) == 0;
}

inline std::string human_string(int n){
	static const std::string names [] = {"", "K", "M", "G"};
	int i = 0;
	while(n > 1000 && i < sizeof(names)){
		n /= 1000;
		++i;
	}
	return std::to_string(n) + names[i];
}

inline std::string field(std::string const & str){
	const int length = 12;
	if(str.size() > length) return str + ":\t";
	auto add = length - str.size();
	return str + ":" + std::string(add, ' ') + "\t";
}
