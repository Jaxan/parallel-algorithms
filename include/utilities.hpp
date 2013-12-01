#pragma once

#include <string>
#include <chrono>

inline bool is_pow_of_two(int n){
	return (n & (n - 1)) == 0;
}

// calculates integer 2-log such that:
// 2^(two_log(x)) >= x > 2^(two_log(x) - 1)
unsigned int two_log(unsigned int x){
	if(x <= 1) return 0;
	return 8*sizeof(unsigned int) - __builtin_clz(x-1);
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

struct timer{
	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::time_point<clock> time;
	typedef std::chrono::duration<double> seconds;

	std::string name;
	time begin;

	timer(std::string name)
	: name(name)
	, begin(clock::now())
	{}

	~timer(){
		time end = clock::now();
		seconds elapsed = end - begin;
		std::cout << name << "\t" << elapsed.count() << std::endl;
	}
};
