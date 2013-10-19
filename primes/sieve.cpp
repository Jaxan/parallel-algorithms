#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

#include <time.h>

int main(int argc, char **argv){
	int n = (argc > 1) ? std::atoi(argv[1]) : 1000;

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

	// enjoy prime twins
	std::vector<int> prime_twins;
	int current_prime = 2;
	for(int i = 2; i <= n; i++){
		if(!not_prime[i] && i - current_prime == 2) prime_twins.push_back(current_prime);
		if(!not_prime[i]) current_prime = i;
	}
	double time1 = clock() / (CLOCKS_PER_SEC / 1000);

	for(int i = 0; i < prime_twins.size(); ++i) std::cout << prime_twins[i] << std::endl;
	std::cerr << "finding_twins " << (time1 - time0)/1000.0 << std::endl;
}
