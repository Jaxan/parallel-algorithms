#include <vector>
#include <algorithm>
#include <numeric>
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

void print_results(int* array, int n){
	for(int i = 0; i < n; ++i){
		if(array[i]) printf("%d\n", array[i]);
	}
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

	// gather primes locally
	std::vector<int> primes;
	if(s==0){
		for(int i = 2; i <= sn; ++i){
			if(!not_prime[i]) primes.push_back(i);
		}
	}
	for(int i = sn + 1; i < local_n; ++i){
		if(!not_prime[i]) primes.push_back(first + i - sn - 1);
	}

	// We share the number of primes each processor found
	// so that we can allocate the minimum number of ints.
	int my_size = primes.size();
	std::vector<int> sizes(p, 0);
	bsp::push_reg(sizes.data(), p);
	bsp::sync();

	for(int t = 0; t < p; ++t) bsp::put(t, &my_size, sizes.data(), s, 1);
	bsp::sync();

	// Allocate total number of primes
	int sum = std::accumulate(sizes.begin(), sizes.end(), 0);
	std::vector<int> prime_array(sum, 0);
	bsp::push_reg(prime_array.data(), sum);
	bsp::sync();

	// Send them all to all, offset is partial sum of sizes
	int offset = std::accumulate(sizes.begin(), sizes.begin() + s, 0);
	for(int t = 0; t < p; ++t) bsp::put(t, primes.data(), prime_array.data(), offset, primes.size());
	bsp::sync();
	primes.clear();

	// For goldbach we can check up to n, we only have to check evens
	// So we will check n/2-1 (excluding 0 and 2) integers, cyclically
	// Every proc will search for an counterxample
	int counterexample = 0;
	int goldbach_n = ((n/2-1)+p-s-1)/p;
	for(int i = 0; i < goldbach_n; ++i){
		counterexample = 2*(i*p + s) + 4;
		std::vector<int>::const_iterator small_prime = prime_array.begin();
		std::vector<int>::const_iterator big_prime = prime_array.end()-1;
		while(small_prime <= big_prime){
			if(*small_prime + *big_prime > counterexample) big_prime--;
			if(*small_prime + *big_prime < counterexample) small_prime++;
			if(*small_prime + *big_prime == counterexample) {
				// Oh, it was no counterexample!
				counterexample = 0;
				break;
			}
		}
		if(counterexample) break;
	}

	if(counterexample) printf("Processor %d found the following counterexample: %d\n", s, counterexample);
	else printf("Processor %d found no counterexample\n", s);
	prime_array.clear();

	bsp::sync();
	bsp::end();
}

int main(int argc, char **argv){
	bsp::init(sieve, argc, argv);
	N = (argc > 1) ? std::atoi(argv[1]) : 1000;
	P = (argc > 2) ? std::atoi(argv[2]) : bsp::nprocs();

	sieve();
}
