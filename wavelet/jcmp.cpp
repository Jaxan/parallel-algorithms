#include <includes.hpp>
#include <utilities.hpp>

#include <boost/filesystem.hpp>
#include <png.hpp>

#include "jcmp.hpp"
#include "wavelet.hpp"

using namespace wvlt::V2;

// Can be any functions from R+ -> R
// as long as backward is its inverse
static double forward(double x){
	//return std::log(1 + std::log(1 + x));
	return std::log(x);
}

static double backward(double x){
	//return std::exp(std::exp(x) - 1) - 1;
	return std::exp(x);
}

// note: we take a copy, because we will modify it in place
static jcmp::image compress(std::vector<double> image, jcmp::uint width, double threshold, unsigned int& nzeros){
	jcmp::uint height = image.size() / width;
	assert(is_pow_of_two(width));
	assert(is_pow_of_two(height));

	// wavelet transform in x-direction
	for(unsigned int i = 0; i < height; ++i){
		wavelet(&image[i*width], width, 1);
	}

	// wavelet transform in y-direction
	for(unsigned int i = 0; i < width; ++i){
		wavelet(&image[i], height, width);
	}

	double min_abs = 10000.0;
	double max_abs = 0.0;

	for(auto& el : image){
		auto absel = std::abs(el);
		if(absel > threshold) {
			min_abs = std::min(min_abs, absel);
			max_abs = std::max(max_abs, absel);
		} else {
			el = 0;
		}
	}

	jcmp::quantization q(&forward, &backward, max_abs, min_abs);

	// save the principal coefficients
	std::vector<jcmp::coefficient> v;
	for(unsigned int y = 0; y < height; ++y){
		for(unsigned int x = 0; x < width; ++x){
			auto&& el = image[x + width*y];
			if(el != 0) v.push_back({q.forwards(el), jcmp::uint(x), jcmp::uint(y)});
		}
	}

	nzeros = v.size();
	return jcmp::image(width, height, q.p, std::move(v));
}

static std::vector<double> decompress(jcmp::image in){
	auto width = in.header.width;
	auto height = in.header.height;
	assert(is_pow_of_two(width));
	assert(is_pow_of_two(height));

	auto q = in.header.get_quantization(&forward, &backward);

	std::vector<double> image(width * height, 0.0);

	// read in coefficient on coordinates
	for(auto it = in.data.begin(); it != in.data.end(); ++it){
		auto&& x = *it;
		image[x.x + width*x.y] = q.backwards(x.c);
	}

	in.clear();

	// inverse wavelet transform in y-direction
	for(unsigned int i = 0; i < width; ++i){
		unwavelet(&image[i], height, width);
	}

	// inverse wavelet transform in x-direction
	for(unsigned int i = 0; i < height; ++i){
		unwavelet(&image[i*width], width, 1);
	}

	return image;
}

int main(){
	namespace fs = boost::filesystem;

	fs::path directory("images");
	fs::directory_iterator eod;
	for(fs::directory_iterator it(directory); it != eod; ++it){
		auto && path = it->path();
		if(path.extension() != ".png") continue;

		// open file
		std::string filename = path.string();
		std::cout << field("file") << filename << std::endl;
		png::istream image(filename);

		auto width = image.get_width();
		auto height = image.get_height();

		// read into vector
		std::vector<double> image_vec;
		image_vec.reserve(width * height);
		for(unsigned char c = 0; image >> c;) image_vec.push_back(c/255.0);

		// compress and decompress to see how we lost information
		unsigned int nzeros = 0;
		auto compressed_vec = decompress(compress(image_vec, width, 0.1, nzeros));

		// output some information
		std::cout << field("raw") << human_string(sizeof(uint8_t) * image_vec.size(), "b") << std::endl;
		std::cout << field("compressed") << human_string(sizeof(jcmp::coefficient) * nzeros, "b") << std::endl;

		// save to output file
		std::string cfilename = "compressed/" + path.filename().string();
		png::gray_ostream compressed_image(width, height, cfilename);
		for(unsigned int i = 0; i < compressed_vec.size(); ++i) compressed_image << compressed_vec[i];
	}
}
