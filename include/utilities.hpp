#pragma once

#include <string>
#include <chrono>

template <typename Int>
bool is_pow_of_two(Int n){
	return n && !(n & (n - 1));
}

template <typename Int>
bool is_even(Int n){
	return (n & 1) == 0;
}

// calculates integer 2-log such that:
// 2^(two_log(x)) >= x > 2^(two_log(x) - 1)
inline unsigned int two_log(unsigned int x){
	if(x <= 1) return 0;
	return 8*sizeof(unsigned int) - unsigned(__builtin_clz(x-1));
}

// calculates 2^x (by squaring)
inline unsigned int pow_two(unsigned int x){
	unsigned int base = 2;
	unsigned int y = 1;
	while(x){
		if(x & 1) y *= base;
		x >>= 1;
		base *= base;
	}
	return y;
}

// Makes numbers human-readable with one decimal
// eg 2350000 becomes 2.3M
template <typename Int>
inline std::string human_string(Int n, std::string suffix = ""){
	static const std::string names [] = {"", "K", "M", "G"};
	unsigned int i = 0;
	Int m = 10*n;
	while(n > 1000 && i < sizeof(names)){
		n /= 1000;
		m /= 1000;
		++i;
	}
	// cast is to make the old gcc 4.4 happy (it doesn't have all overloads of to_string)
	return std::to_string(n) + "." + std::to_string(m % 10) + names[i] + suffix;
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
void print_vec(std::vector<T> const & v){
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

	timer(std::string name_)
	: name(name_)
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
	inline std::string red(std::string s){
		return "\x1b[31m" + s + "\x1b[39m";
	}
	inline std::string green(std::string s){
		return "\x1b[32m" + s + "\x1b[39m";
	}
}
