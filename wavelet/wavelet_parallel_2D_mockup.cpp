#include <includes.hpp>
#include <iomanip>
#include <utilities.hpp>
#include <boost/program_options.hpp>
#include <bsp.hpp>

#include "wavelet.hpp"
#include "wavelet_parallel.hpp"

static struct {
	unsigned int P;
	unsigned int W;
	unsigned int H;
	unsigned int M;
	bool check_results;
} globals;

static std::vector<double> seqr;
static std::vector<double> parr;

using namespace wvlt;

struct plan_2D {
	par::plan_1D horizontal;
	par::plan_1D vertical;
};

struct block {
	std::vector<double> data;
	std::vector<double> hcomm;
	std::vector<double> vcomm;

	block(plan_2D const & plan)
	: data(plan.horizontal.b * plan.vertical.b, 0.0)
	, hcomm(plan.horizontal.Cm * plan.vertical.b, 0.0)
	, vcomm(plan.horizontal.b * plan.vertical.Cm, 0.0)
	{}

	void push() {
		bsp::push_reg(data.data(), data.size());
		bsp::push_reg(hcomm.data(), hcomm.size());
		bsp::push_reg(vcomm.data(), vcomm.size());
	}

	void pop() {
		bsp::pop_reg(data.data());
		bsp::pop_reg(hcomm.data());
		bsp::pop_reg(vcomm.data());
	}
};

// Communicate vertical data from b (strided) to b2 (not strided)
static void vcomm_step(wvlt::par::proc_info const & pi, plan_2D const & plan, block const & b, block & b2, unsigned int stride){
	const unsigned int Cm = plan.vertical.Cm;
	const unsigned int width = plan.horizontal.b;
	for(unsigned int i = 0; i < Cm; ++i){
		for(unsigned int x = 0; x < width; ++x){
			bsp::put(pi.prev, b.data.data() + x + width*stride*i, b2.vcomm.data(), x + width*i, 1);
		}
	}
}

// Communicate horizontal data from b (strided) to b2 (not strided)
static void hcomm_step(wvlt::par::proc_info const & pi, plan_2D const & plan, block const & b, block & b2, unsigned int stride){
	const unsigned int Cm = plan.horizontal.Cm;
	const unsigned int width = plan.horizontal.b;
	for(unsigned int y = 0; y < plan.vertical.b; ++y){
		for(unsigned int i = 0; i < Cm; ++i){
			bsp::put(pi.prev, b.data.data() + width*y + stride*i, b2.hcomm.data(), Cm*y + i, 1);
		}
	}
}

static void vcomp_step(plan_2D const & plan, block & b, unsigned int stride){
	const unsigned int width = plan.horizontal.b;
	const unsigned int height = plan.vertical.b;
	unsigned int end = pow_two(plan.vertical.m);
	for(unsigned int x = 0; x < width; ++x){
		for(unsigned int i = 1; i < end; i <<= 1){
			wavelet_mul(b.data.data() + x, b.vcomm[x], b.vcomm[x + width*i], width*height, width*stride*i);
			if(i < end/2) wavelet_mul_base(&b.vcomm[x], width*(2*end - 2*i), width*i);
		}
	}
}

static void hcomp_step(plan_2D const & plan, block & b, unsigned int stride){
	const unsigned int width = plan.horizontal.b;
	const unsigned int height = plan.vertical.b;
	unsigned int end = pow_two(plan.horizontal.m);
	for(unsigned int y = 0; y < height; ++y){
		for(unsigned int i = 1; i < end; i <<= 1){
			auto x0 = b.hcomm[plan.horizontal.Cm*y];
			auto x1 = b.hcomm[plan.horizontal.Cm*y + i];
			wavelet_mul(b.data.data() + width*y, x0, x1, width, stride*i);
			if(i < end/2) wavelet_mul_base(&b.hcomm[plan.horizontal.Cm*y], 2*end - 2*i, i);
		}
	}
}

static void vstep(wvlt::par::proc_info const & pi, plan_2D const & plan, std::vector<block> & blocks, unsigned int stride){
	for(unsigned int b = 0; b < blocks.size(); ++b){
		unsigned int b2 = b;
		vcomm_step(pi, plan, blocks[b], blocks[b2], stride);
	}
	bsp::sync();
	for(unsigned int b = 0; b < blocks.size(); ++b){
		vcomp_step(plan, blocks[b], stride);
	}
}

static void hstep(wvlt::par::proc_info const & pi, plan_2D const & plan, std::vector<block> & blocks, unsigned int stride){
	for(unsigned int b = 0; b < blocks.size(); ++b){
		unsigned int b2 = (b-1+blocks.size()) % blocks.size();
		hcomm_step(pi, plan, blocks[b], blocks[b2], stride);
	}
	bsp::sync();
	for(unsigned int b = 0; b < blocks.size(); ++b){
		hcomp_step(plan, blocks[b], stride);
	}
}

// gets globals from processor 0
static void get_globals(){
	bsp::push_reg(&globals);
	bsp::sync();
	bsp::get(0, &globals, 0, &globals);
	bsp::sync();
	bsp::pop_reg(&globals);
}

// fake data
double data(unsigned int x, unsigned int y){
	return x*y;
}

static void par_wavelet_2D(){
	bsp::begin(globals.P);
	get_globals();

	const wvlt::par::proc_info d(bsp::nprocs(), bsp::pid());
	const wvlt::par::plan_1D horizontal(globals.W, globals.W/d.p, globals.M);
	const wvlt::par::plan_1D vertical(globals.H, globals.H/d.p, globals.M);
	const plan_2D plan{horizontal, vertical};

	auto bbb = block(plan);
	// We allocated everything up front, we don't actually need to do this
	// but it's easy, as we don't have to think of this later.
	std::vector<block> blocks(d.p, bbb);
	std::vector<double> hfinish(2 * d.p * vertical.b, 0.0);
	std::vector<double> vfinish(horizontal.b * 2 * d.p, 0.0);

	// As we will be generating the data, no need to sync
	for(unsigned int b = 0; b < blocks.size(); ++b){
		unsigned int x_start = b * horizontal.b;
		unsigned int y_start = (d.s - b + d.p)%d.p * vertical.b;
		for(unsigned int y = 0; y < vertical.b; ++y){
			for(unsigned int x = 0; x < horizontal.b; ++x){
				auto v = data(x_start + x, y_start + y);
				blocks[b].data[x + horizontal.b*y] = v;
			}
		}
	}

	for(auto & b : blocks) b.push();
	bsp::push_reg(hfinish.data(), hfinish.size());
	bsp::push_reg(vfinish.data(), vfinish.size());
	bsp::sync();

	double time1 = bsp::time();

	if(true){ // horizontal
		// do local wavelets and communications
		unsigned int stride = 1;
		for(unsigned int i = 0; i < horizontal.big_steps; ++i){
			hstep(d, plan, blocks, stride);
			stride <<= plan.horizontal.m;
		}

		// fan in to the right processor
		unsigned int hh = horizontal.b/2;
		for(unsigned int b = 0; b < blocks.size(); ++b){
			unsigned int t = (d.s - b + d.p)%d.p;
			unsigned int x_start = b * 2;
			auto ptr = blocks[b].data.data();
			for(unsigned int y = 0; y < vertical.b; ++y){
				// processor, source, dest, offset
				bsp::put(t, &ptr[0  + horizontal.b*y], hfinish.data(), 0 + x_start + 2*d.p*y);
				bsp::put(t, &ptr[hh + horizontal.b*y], hfinish.data(), 1 + x_start + 2*d.p*y);
			}
		}
		bsp::sync();

		// last step of the algorithm
		for(unsigned int y = 0; y < vertical.b; ++y){
			wvlt::wavelet(hfinish.data() + 2*d.p*y, 2*d.p, 1);
		}

		// fan out to the right processor
		for(unsigned int y = 0; y < vertical.b; ++y){
			for(unsigned int t = 0; t < d.p; ++t){
				unsigned int b = (t - d.s + d.p)%d.p;
				unsigned int x_start = b * 2;
				bsp::put(t, &hfinish[0 + x_start + 2*d.p*y], blocks[b].data.data(), 0  + horizontal.b*y);
				bsp::put(t, &hfinish[1 + x_start + 2*d.p*y], blocks[b].data.data(), hh + horizontal.b*y);
			}
		}
		bsp::sync();
	}

	if(true){ //vertical
		// do local wavelets and communications in other directions
		unsigned int stride = 1;
		for(unsigned int i = 0; i < vertical.big_steps; ++i){
			vstep(d, plan, blocks, stride);
			stride <<= plan.vertical.m;
		}

		// fan in to the right processor
		unsigned int hh = vertical.b/2;
		for(unsigned int b = 0; b < blocks.size(); ++b){
			unsigned int t = b;
			unsigned int y_start = (d.s - b + d.p)%d.p * 2;
			auto ptr = blocks[b].data.data();
			for(unsigned int x = 0; x < horizontal.b; ++x){
				// processor, source, dest, offset
				bsp::put(t, &ptr[x + 0 *horizontal.b], vfinish.data(), x + horizontal.b*(y_start + 0));
				bsp::put(t, &ptr[x + hh*horizontal.b], vfinish.data(), x + horizontal.b*(y_start + 1));
			}
		}
		bsp::sync();

		// last step of the algorithm
		for(unsigned int x = 0; x < horizontal.b; ++x){
			wvlt::wavelet(vfinish.data() + x, 2*d.p, horizontal.b);
		}

		// fan out to the right processor
		for(unsigned int x = 0; x < horizontal.b; ++x){
			for(unsigned int t = 0; t < d.p; ++t){
				unsigned int b = d.s;
				unsigned int y_start = (t - b + d.p)%d.p * 2;
				bsp::put(t, &vfinish[x + horizontal.b*(y_start + 0)], blocks[b].data.data(), horizontal.b*0  + x);
				bsp::put(t, &vfinish[x + horizontal.b*(y_start + 1)], blocks[b].data.data(), horizontal.b*hh + x);
			}
		}
		bsp::sync();
	}

	double time2 = bsp::time();
	if(d.s==0) printf("parallel version\t%f\n", time2 - time1);

	if(globals.check_results){
		bsp::push_reg(parr.data(), parr.size());
		bsp::sync();

		for(unsigned int b = 0; b < blocks.size(); ++b){
			unsigned int x_start = b * horizontal.b;
			unsigned int y_start = (d.s - b + d.p)%d.p * vertical.b;
			for(unsigned int y = 0; y < vertical.b; ++y){
				for(unsigned int x = 0; x < horizontal.b; ++x){
					auto v = blocks[b].data[x + horizontal.b*y];
					bsp::put(0, &v, parr.data(), x_start + x + horizontal.n*(y_start+y));
				}
			}
		}
		bsp::sync();
	}

	bsp::end();
}

static void seq_wavelet_2D(){
	for(unsigned int y = 0; y < globals.H; ++y)
		for(unsigned int x = 0; x < globals.W; ++x)
			seqr[x + globals.W*y] = data(x, y);

	auto time1 = timer::clock::now();
	wvlt::wavelet_2D(seqr.data(), globals.W, globals.H);
	auto time2 = timer::clock::now();
	printf("sequential version\t%f\n", timer::from_dur(time2 - time1));
}

static double sq_diff(double x, double y){ return (x-y)*(x-y); }
static int compare_results(std::vector<double> const & lh, std::vector<double> const & rh, double threshold){
	if(lh == rh){
		std::cout << colors::green("SUCCES:") << " bitwise qual" << std::endl;
		return 0;
	}

	double rmse = std::sqrt(std::inner_product(lh.begin(), lh.end(), rh.begin(), 0.0, std::plus<double>(), &sq_diff) / lh.size());
	if(rmse <= threshold){
		std::cout << colors::green("SUCCES:") << " error within threshold, rmse = " << rmse << std::endl;
		return 1;
	} else {
		std::cout << colors::red("FAIL:") << " error to big, rmse = " << rmse << std::endl;
		return 2;
	}
}

int main(int argc, char** argv){
	bsp::init(par_wavelet_2D, argc, argv);
	namespace po = boost::program_options;

	// Describe program options
	po::options_description opts;
	opts.add_options()
		("p", po::value<unsigned int>(), "number of processors")
		("w", po::value<unsigned int>(), "width of image")
		("h", po::value<unsigned int>(), "height of image")
		("m", po::value<unsigned int>()->default_value(1), "the variable m")
		("help", po::bool_switch(), "show this help")
		("show-input", po::bool_switch(), "shows the given input")
		("seq", po::bool_switch(), "also runs the sequential algorithm")
		("check", po::bool_switch(), "enables correctness checks");
	po::variables_map vm;

	// Parse and set options
	try {
		po::store(po::parse_command_line(argc, argv, opts), vm);
		po::notify(vm);

		if(vm["help"].as<bool>()){
			std::cout << "Parallel wavelet mockup" << std::endl;
			std::cout << opts << std::endl;
			return 0;
		}

		globals.P = vm["p"].as<unsigned int>();
		globals.W = vm["w"].as<unsigned int>();
		globals.H = vm["h"].as<unsigned int>();
		globals.M = vm["m"].as<unsigned int>();
		globals.check_results = vm["check"].as<bool>();

		if(!is_pow_of_two(globals.P)) throw po::error("p is not a power of two");
		if(!is_pow_of_two(globals.W)) throw po::error("w is not a power of two");
		if(!is_pow_of_two(globals.H)) throw po::error("h is not a power of two");
	} catch(std::exception& e){
		std::cout << colors::red("ERROR: ") << e.what() << std::endl;
		std::cout << opts << std::endl;
		return 1;
	}

	if(vm["show-input"].as<bool>()){
		std::cout << "w\t" << globals.W << "h\t" << globals.H << "\np\t" << globals.P << "\nm\t" << globals.M << std::endl;
	}

	seqr.assign(globals.W*globals.H, 0.0);
	parr.assign(globals.W*globals.H, 0.0);

	par_wavelet_2D();
	seq_wavelet_2D();

	if(globals.check_results){
		double threshold = 1.0e-8;
		std::cout << "Checking results ";
		compare_results(seqr, parr, threshold);
	}
}
