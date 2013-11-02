#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

#include <time.h>

struct options_t {
	int n;
	bool use_twins;
	bool output_list;
	bool test_goldbach;
} options;

int main(int argc, char **argv){
	options.n = (argc > 1) ? std::atoi(argv[1]) : 1000;
	options.use_twins = false;
	options.output_list = false;
	options.test_goldbach = false;

	int n = options.n;
	assert(n > 0);

	// sets all to false
	std::vector<bool> not_prime(n+1);

	// sieve
	int sn = std::sqrt(n);
	double time0 = clock() / (CLOCKS_PER_SEC / 1000);
	for(int i = 2; i <= sn; i++){
		// goto first prime (i = sn+1 does not matter, as we start at a multiple)
		while(i <= sn && not_prime[i]) i++;

		// cross out multiples
		for(int j = i*i; j <= n; j += i) not_prime[j] = true;
	}
	double time1 = clock() / (CLOCKS_PER_SEC / 1000);

	// gather primes
	std::vector<int> primes;
	if(options.use_twins){
		for(int i = 2; i <= n-2; i++)
			if(!not_prime[i] && !not_prime[i+2]) primes.push_back(i);
	} else {
		for(int i = 2; i <= n; i++)
			if(!not_prime[i]) primes.push_back(i);
	}
	double time2 = clock() / (CLOCKS_PER_SEC / 1000);
	not_prime.clear();

	// output
	if(options.output_list)
		for(int i = 0; i < primes.size(); ++i)
			printf("%d\n", primes[i]);

	// do the goldbach thing
	if(options.test_goldbach){
		// For goldbach we can check up to n, we only have to check evens
		// So we will check n/2-1 (excluding 0 and 2) integers
		int counterexample = 0;
		for(int i = 4; i <= n; i += 2){
			counterexample = i;
			std::vector<int>::const_iterator small_prime = primes.begin();
			std::vector<int>::const_iterator big_prime = primes.end()-1;
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

		if(counterexample) printf("found the following counterexample: %d\n", counterexample);
		else printf("no counterexample found\n");
	}

	printf("sieving %f\n", (time1 - time0)/1000.0);
	printf("gathering %f\n", (time2 - time0)/1000.0);
}
