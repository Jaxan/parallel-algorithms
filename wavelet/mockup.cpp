#include <includes.hpp>
#include <utilities.hpp>
#include <bsp.hpp>

#include "wavelet2.hpp"
#include "defines.hpp"

#ifndef NEXP
// will take about 1.3 GB
#define NEXP 25
#endif

const unsigned int P = 2;
const unsigned int N = 1 << NEXP;

// Static vectors for correctness checking
static std::vector<double> par_result(N);
static std::vector<double> seq_result(N);

// Convenience container of some often-used values
// NOTE: we use block distribution
// n = inputisze, p = nproc(), s = pid(), b = blocksize
struct distribution {
	unsigned int n, p, s, b;
};

// fake data
static double data(unsigned int global_index){
	return global_index - N/2.0 + 0.5 + std::sin(0.1337*global_index);
}

// NOTE: does not synchronize
static void read_and_distribute_data(distribution const & d, double* x){
	std::vector<double> r(d.b);
	for(unsigned int t = 0; t < d.p; ++t){
		r.assign(d.b, 0.0);
		for(unsigned int i = 0; i < d.b; ++i){
			r[i] = data(i + t*d.b);
		}
		bsp::put(t, r.data(), x, 0, r.size());
	}
}

// NOTE: we assume x, next and proczero to be already allocated and bsp::pushed
// NOTE: no sync at the end
// block distributed parallel wavelet, result is also in block distribution (in-place in x)
static void par_wavelet_base(distribution const & d, double* x, double* next, double* proczero){
	for(unsigned int i = 1; i <= d.b/4; i <<= 1){
		// send the two elements over
		auto t = (d.s - 1 + d.p) % d.p;
		bsp::put(t, &x[0], next, 0);
		bsp::put(t, &x[i], next, 1);
		bsp::sync();

		wvlt::wavelet_mul(x, next[0], next[1], d.b, i);
	}

	// fan in (i.e. 2 elements to proc zero)

	bsp::sync();

	// send 2 of your own elements
	for(unsigned int i = 0; i < 2; ++i){
		bsp::put(0, &x[i * d.b/2], proczero, d.s * 2 + i);
	}
	bsp::sync();

	// proc zero has the privilige/duty to finish the job
	if(d.s == 0) {
		wvlt::wavelet(proczero, 2*d.p);
		// and to send it back to everyone
		for(unsigned int t = 0; t < d.p; ++t){
			for(unsigned int i = 0; i < 2; ++i){
				bsp::put(t, &proczero[t*2 + i], x, i * d.b/2);
			}
		}
	}
}

static void par_wavelet(){
	bsp::begin(P);
	distribution d{N, bsp::nprocs(), bsp::pid(), N/P};

	// We allocate and push everything up front, since we need it anyways
	// (so peak memory is the same). This saves us 1 bsp::sync
	// For convenience and consistency we use std::vector
	std::vector<double> x(d.b, 0.0);
	std::vector<double> next(2, 0.0);
	std::vector<double> proczero(d.s == 0 ? 2*d.p : 1, 0.0);

	bsp::push_reg(x.data(), x.size());
	bsp::push_reg(next.data(), next.size());
	bsp::push_reg(proczero.data(), proczero.size());

	bsp::sync();

	// processor zero reads data from file
	// gives each proc its own piece
	if(d.s == 0) read_and_distribute_data(d, x.data());
	bsp::sync();

	double time1 = bsp::time();

	par_wavelet_base(d, x.data(), next.data(), proczero.data());
	bsp::sync();

	double time2 = bsp::time();
	if(d.s==0) printf("parallel version\t%f\n", time2 - time1);

	// Clean up and send all data to proc zero for correctness checking
	// So this is not part of the parallel program anymore
	bsp::pop_reg(proczero.data());
	bsp::pop_reg(next.data());
	next.clear();
	proczero.clear();

	bsp::push_reg(par_result.data(), par_result.size());
	bsp::sync();

	bsp::put(0, x.data(), par_result.data(), d.s * d.b, d.b);
	bsp::sync();

	bsp::pop_reg(par_result.data());
	bsp::pop_reg(x.data());
	bsp::end();
}

static void seq_wavelet(){
	std::vector<double> v(N);
	for(unsigned int i = 0; i < N; ++i) v[i] = data(i);

	{	auto time1 = timer::clock::now();
		wvlt::wavelet(v.data(), v.size());
		auto time2 = timer::clock::now();
		printf("sequential version\t%f\n", timer::from_dur(time2 - time1));
	}

	std::copy(v.begin(), v.end(), seq_result.begin());
}

// Checks whether seq and par agree
// NOTE: modifies the global par_result
static void check_equality(double threshold){
	if(par_result == seq_result){
		std::cout << colors::green("SUCCES:") << " Results are bitwise equal" << std::endl;
	} else {
		for(unsigned int i = 0; i < N; ++i){
			auto sq = par_result[i] - seq_result[i];
			par_result[i] = sq*sq;
		}
		auto rmse = std::sqrt(std::accumulate(par_result.begin(), par_result.end(), 0.0) / N);
		if(rmse <= threshold){
			std::cout << colors::green("SUCCES:") << " Results are almost the same: rmse = " << rmse << std::endl;
		} else {
			std::cout << colors::red("FAIL:") << " Results differ: rmse = " << rmse << std::endl;
		}
	}
}

// Checks whether inverse gives us the data back
// NOTE: modifies the global seq_result
static void check_inverse(double threshold){
	wvlt::unwavelet(seq_result.data(), seq_result.size());
	bool same = true;
	for(unsigned int i = 0; i < N; ++i){
		if(data(i) != seq_result[i]) same = false;
		auto sq = data(i) - seq_result[i];
		seq_result[i] = sq*sq;
	}
	auto rmse = std::sqrt(std::accumulate(seq_result.begin(), seq_result.end(), 0.0) / N);
	if(same){
		std::cout << colors::green("SUCCES:") << " Inverse is bitwise correct" << std::endl;
	} else {
		if(rmse <= threshold){
			std::cout << colors::green("SUCCES:") << " Inverse are almost correct: rmse = " << rmse << std::endl;
		} else {
			std::cout << colors::red("FAIL:") << " Inverse seems wrong: rmse = " << rmse << std::endl;
		}
	}
}

int main(int argc, char** argv){
	bsp::init(par_wavelet, argc, argv);

	// Run both versions (will print timings)
	seq_wavelet();
	par_wavelet();

	// Checking equality of algorithms
	bool should_check = false;
	if(should_check){
		double threshold = 1.0e-8;
		check_equality(threshold);
		check_inverse(threshold);
	}
}
