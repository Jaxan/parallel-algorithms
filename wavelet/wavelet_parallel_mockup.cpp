#include <includes.hpp>
#include <utilities.hpp>
#include <boost/program_options.hpp>
#include <bsp.hpp>

#include "wavelet.hpp"
#include "wavelet_parallel.hpp"

// Number of iterations to improve time measurements
static unsigned int ITERS = 1;

// Static :(, will be set in main
static unsigned int P;
static unsigned int N;

// Static vectors for correctness checking
static std::vector<double> par_result;
static std::vector<double> seq_result;

// fake data
static double data(unsigned int global_index){
	return global_index - N/2.0 + 0.5 + std::sin(0.1337*global_index);
}

// NOTE: does not synchronize
static void read_and_distribute_data(wvlt::par::distribution const & d, double* x){
	std::vector<double> r(d.b);
	for(unsigned int t = 0; t < d.p; ++t){
		r.assign(d.b, 0.0);
		for(unsigned int i = 0; i < d.b; ++i){
			r[i] = data(i + t*d.b);
		}
		bsp::put(t, r.data(), x, 0, r.size());
	}
}

static void par_wavelet(){
	bsp::begin(P);
	wvlt::par::distribution d(N, bsp::nprocs(), bsp::pid());

	unsigned int m = 2;
	unsigned int Cm = wvlt::par::communication_size(m);

	// We allocate and push everything up front, since we need it anyways
	// (so peak memory is the same). This saves us 1 bsp::sync()
	// For convenience and consistency we use std::vector
	std::vector<double> x(d.b, 0.0);
	std::vector<double> next(Cm, 0.0);
	std::vector<double> proczero(d.s == 0 ? 2*d.p : 1, 0.0);

	bsp::push_reg(x.data(), x.size());
	bsp::push_reg(next.data(), next.size());
	bsp::push_reg(proczero.data(), proczero.size());

	bsp::sync();

	// processor zero reads data from file
	// gives each proc its own piece
	if(d.s == 0) read_and_distribute_data(d, x.data());
	bsp::sync();

	// do the parallel wavelet!!!
	double time1 = bsp::time();
	for(unsigned int i = 0; i < ITERS; ++i){
		wvlt::par::wavelet(d, x.data(), next.data(), proczero.data(), m);
		bsp::sync();
	}
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
		for(unsigned int i = 0; i < ITERS; ++i){
			wvlt::wavelet(v.data(), v.size(), 1);
		}
		auto time2 = timer::clock::now();
		printf("sequential version\t%f\n", timer::from_dur(time2 - time1));
	}

	std::copy(v.begin(), v.end(), seq_result.begin());
}

// square difference, used to calculate root mean squared error
static double sq_diff(double x, double y){ return (x-y)*(x-y); }

static void compare_results(std::vector<double> const & lh, std::vector<double> const & rh, double threshold){
	if(lh == rh){
		std::cout << colors::green("SUCCES:") << " bitwise qual" << std::endl;
		return;
	}

	double rmse = std::sqrt(std::inner_product(lh.begin(), lh.end(), rh.begin(), 0.0, std::plus<double>(), &sq_diff) / lh.size());
	if(rmse <= threshold){
		std::cout << colors::green("SUCCES:") << " error within threshold, rmse = " << rmse << std::endl;
	} else {
		std::cout << colors::red("FAIL:") << " error to big, rmse = " << rmse << std::endl;
	}
}

int main(int argc, char** argv){
	namespace po = boost::program_options;

	// Describe program options
	po::options_description opts;
	opts.add_options()
		("p", po::value<unsigned int>(), "number of processors")
		("n", po::value<unsigned int>(), "number of elements")
		("iterations", po::value<unsigned int>()->default_value(5), "number of iterations")
		("help", po::value<bool>(), "show this help")
		("check", po::value<bool>(), "enables correctness checks");
	po::variables_map vm;

	// Parse and set options
	try {
		po::store(po::parse_command_line(argc, argv, opts), vm);
		po::notify(vm);

		if(vm.count("help")){
			std::cout << "Parallel wavelet mockup" << std::endl;
			std::cout << opts << std::endl;
			return 0;
		}

		N = vm["n"].as<unsigned int>();
		P = vm["p"].as<unsigned int>();
		ITERS = vm["iterations"].as<unsigned int>();

		if(!is_pow_of_two(N)) throw po::error("n is not a power of two");
		if(!is_pow_of_two(P)) throw po::error("p is not a power of two");
	} catch(std::exception& e){
		std::cout << colors::red("ERROR: ") << e.what() << std::endl;
		std::cout << opts << std::endl;
		return 1;
	}

	// Initialise stuff
	par_result.assign(N, 0.0);
	seq_result.assign(N, 0.0);
	bsp::init(par_wavelet, argc, argv);

	// Run both versions (will print timings)
	seq_wavelet();
	par_wavelet();

	// Checking equality of algorithms
	if(vm.count("check")){
		double threshold = 1.0e-8;
		std::cout << "Checking results ";
		compare_results(seq_result, par_result, threshold);

		for(int i = 0; i < ITERS; ++i) wvlt::unwavelet(seq_result.data(), seq_result.size(), 1);
		for(unsigned int i = 0; i < par_result.size(); ++i) par_result[i] = data(i);

		std::cout << "Checking inverse ";
		compare_results(seq_result, par_result, threshold);
	}
}
