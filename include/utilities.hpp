#pragma once

#include <string>
#include <chrono>

template <typename Int>
bool is_pow_of_two(Int n){
	return (n & (n - 1)) == 0;
}

template <typename Int>
bool is_even(Int n){
	return (n & 1) == 0;
}

// calculates integer 2-log such that:
// 2^(two_log(x)) >= x > 2^(two_log(x) - 1)
unsigned int two_log(unsigned int x){
	if(x <= 1) return 0;
	return 8*sizeof(unsigned int) - __builtin_clz(x-1);
}

// Makes numbers human-readable
// eg 2300000 becomes 2M
inline std::string human_string(int n){
	static const std::string names [] = {"", "K", "M", "G"};
	int i = 0;
	while(n > 1000 && i < sizeof(names)){
		n /= 1000;
		++i;
	}
	// cast is to make the old gcc 4.4 happy (it doesn't have all overloads of to_string)
	return std::to_string((long long)n) + names[i];
}

inline std::string field(std::string const & str){
	const int length = 12;
	if(str.size() > length) return str + ":\t";
	auto add = length - str.size();
	return str + ":" + std::string(add, ' ') + "\t";
}

// Prints a vector with brackets and commas
// Does not work recursively!
template <typename T>
void print_vec(std::vector<T> v){
	auto it = v.begin(), end = v.end();
	std::cout << "{" << *it++;
	while(it != end) std::cout << ", " << *it++;
	std::cout << "}\n";
}

// RAII struct for timing
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
		std::cout << name << "\t" << from_dur(end - begin) << std::endl;
	}

	static double from_dur(seconds s){
		return s.count();
	}
};

namespace colors {
	std::string red(std::string s){
		return "\x1b[31m" + s + "\x1b[39m";
	}
	std::string green(std::string s){
		return "\x1b[32m" + s + "\x1b[39m";
	}
}
