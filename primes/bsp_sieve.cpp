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

struct options_t {
	int n;
	bool use_twins;
	bool output_list;
	bool test_goldbach;
} options;

// gets the options from processor 0 (which has them globally)
options_t get_options(){
	options_t opt = options;
	bsp::push_reg(&opt);
	bsp::sync();

	// get real value from proc 0
	bsp::get(0, &opt, 0, &opt);
	bsp::sync();

	bsp::pop_reg(&opt);
	return opt;
}

int divup(int x, int y){ return (x + y - 1)/y; }
int divdown(int x, int y){ return x/y; }

void sieve(){
	bsp::begin(P);

	int p = bsp::nprocs();
	int s = bsp::pid();

	//  array from 0 to n inclusive
	options_t opt = get_options();
	int n = opt.n;
	int sn = std::sqrt(n);

	// give last processor the excess
	int regular_rest = (n - sn)/p;
	int rest = (s < p-1) ? regular_rest: (n - sn) - (p-1) * regular_rest;

	// local length of array: first part is [0, sn] inclusive
	int local_n = sn + 1 + rest;

	// first number of the second part
	int first = sn + 1 + s * regular_rest;

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
	if(!opt.use_twins){
		// Put all primes in our array
		if(s==0){ // some-one has to do it...
			for(int i = 2; i <= sn; ++i){
				if(!not_prime[i]) primes.push_back(i);
			}
		}
		for(int i = sn + 1; i < local_n; ++i){
			if(!not_prime[i]) primes.push_back(first + i - sn - 1);
		}
	} else {
		// Put only twin primes in our array
		// Note that we have to take care of the boundary
		int boundary = local_n - 1;
		for(; boundary > sn + 1; --boundary){
			if(!not_prime[boundary]) break;
		}
		boundary = boundary + first - sn - 1;
		bsp::push_reg(&boundary, 1);
		bsp::sync();

		if(s < p-1) bsp::put(s + 1, &boundary, &boundary, 0, 1);
		bsp::sync();

		if(s == 0) { // first processors does first sn elements
			for(int i = 2; i <= sn; i++){
				if(!not_prime[i] && !not_prime[i+2]) primes.push_back(i);
			}
		} else { // other processors should check the boundary
			if(first - boundary <= 2 && (!not_prime[sn+1] || !not_prime[sn+2]))
				primes.push_back(boundary);
		}
		for(int i = sn + 1; i < local_n - 2; i++){
			if(!not_prime[i] && !not_prime[i+2]) primes.push_back(i + first - sn - 1);
		}
	}

	bsp::sync();	// only needed for timing
	double time2 = bsp::time();
	not_prime.clear();

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

	if(opt.output_list && s == 0){
		for(int i = 0; i < prime_array.size(); ++i){
			printf("%d\n", prime_array[i]);
		}
	}

	if(opt.test_goldbach){
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
	}
	prime_array.clear();

	if(s==0){
		printf("sieving %f\n", time1 - time0);
		printf("gathering %f\n", time2 - time1);
	}

	bsp::sync();
	bsp::end();
}

int main(int argc, char **argv){
	bsp::init(sieve, argc, argv);

	options.n = (argc > 1) ? std::atoi(argv[1]) : 1000;
	options.use_twins = false;
	options.output_list = false;
	options.test_goldbach = true;

	P = (argc > 2) ? std::atoi(argv[2]) : bsp::nprocs();

	sieve();
}
