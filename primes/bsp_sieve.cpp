#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <bsp.hpp>

int P = 0;
int N = 0;

int get_n(int s){
	int n = N;
	bsp::push_reg(&n);
	bsp::sync();

	// get real value from proc 0
	bsp::get(0, &n, 0, &n);
	bsp::sync();

	bsp::pop_reg(&n);
	return n;
}

int divup(int x, int y){ return (x + y - 1)/y; }
int divdown(int x, int y){ return x/y; }

void sieve(){
	bsp::begin(P);

	int p = bsp::nprocs();
	int s = bsp::pid();

	//  array from 0 to n inclusive
	int n = get_n(s);
	int length = n + 1;
	int sn = std::sqrt(n);

	// give last processor the excess
	int regular_rest = (n - sn)/p;
	int rest = (s < p-1) ? regular_rest: (n - sn) - (p-1) * regular_rest;

	// local length of array: first part is [0, sn] inclusive
	int local_n = sn + 1 + rest;

	// first number of the second part
	int first = sn + 1 + s * regular_rest;

	// debug
	printf("Processor %d: sn = %d, rest = %d, first = %d, local_n = %d\n", s, sn, rest, first, local_n);

	// first part is [0, sn], second part [sn+1, local_n)
	double time0 = bsp::time();
	std::vector<bool> not_prime(local_n);
	for(int i = 2; i <= sn; ++i){
		// goto first prime
		while(i <= sn && not_prime[i]) i++;
		if(i > sn) break;

		// cross out multiples in first part
		for(int j = i*i; j <= sn; j += i) not_prime[j] = true;

		// cross out multiples in second part
		for(int j = i*divup(first, i) - first + sn + 1; j < local_n; j += i) not_prime[j] = true;
	}

	bsp::sync();	// only needed for timing
	double time1 = bsp::time();

	// for twin primes, we have to check the boundary as well
	// send the biggest prime to next proc
	int boundary = local_n - 1;
	for(; boundary > sn + 1; --boundary){
		if(!not_prime[boundary]) break;
	}
	boundary = boundary + first - sn - 1;
	bsp::push_reg(&boundary, 1);
	bsp::sync();

	if(s < p-1) bsp::put(s + 1, &boundary, &boundary, 0, 1);
	bsp::sync();

	// Stores only the first of a prime twin (so the second is +2)
	std::vector<int> prime_twins;

	// scans for twin primes
	int current_prime = (s == 0) ? 2 : boundary;
	if(s == 0) {
		for(int i = 2; i <= sn; i++){
			if(!not_prime[i] && i - current_prime == 2) prime_twins.push_back(current_prime);
			if(!not_prime[i]) current_prime = i;
		}
	}
	for(int i = sn + 1; i < local_n; i++){
		int ri = i + first - sn - 1;
		if(!not_prime[i] && ri - current_prime == 2) prime_twins.push_back(current_prime);
		if(!not_prime[i]) current_prime = ri;
	}

	// proc 0 will be given all primes
	// for x > 16
	// upperbound for primes under n is p(n) < 1.25506 n / log n
	// lowerbound for primes under n is n / log n < p(n)
	int * prime_array = 0;
	if(s == 0){
		prime_array = new int[n]();
	} else {
		prime_array = new int[0]();
	}

	bsp::push_reg(prime_array, s == 0 ? n : 0);
	bsp::sync();

	// Send them all
	bsp::put(0, prime_twins.data(), prime_array, first, prime_twins.size());
	bsp::sync();

	prime_twins.clear();
	if(s == 0){
		for(int i = 0; i < n; ++i){
			if(prime_array[i]) std::cout << prime_array[i] << "\n";
		}
		std::cout << std::endl;
		std::cerr << "sieving " << time1 - time0 << std::endl;
	}

	delete[] prime_array;

	bsp::sync();
	bsp::end();
}

int main(int argc, char **argv){
	bsp::init(sieve, argc, argv);
	N = (argc > 1) ? std::atoi(argv[1]) : 1000;
	P = (argc > 2) ? std::atoi(argv[2]) : bsp::nprocs();

	sieve();
}
