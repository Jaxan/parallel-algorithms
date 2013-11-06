#include <includes.hpp>

#include <boost/filesystem.hpp>
#include <png.hpp>
#include <utilities.hpp>

#include "compressed_image.hpp"
#include "striding_iterator.hpp"
#include "periodic_iterator.hpp"
#include "wavelet.hpp"

// note: we take a copy, because we will modify it in place
jcmp::image compress(std::vector<double> image, int width, double threshold, int& zeros){
	auto height = image.size() / width;
	assert(is_pow_of_two(width));
	assert(is_pow_of_two(height));

	// wavelet transform in x-direction
	for(int i = 0; i < height; ++i){
		wavelet(image.begin() + i*width, image.begin() + (i+1)*width);
	}

	// wavelet transform in y-direction
	for(int i = 0; i < width; ++i){
		wavelet(strided(image.begin() + i, width), strided(image.end() + i, width));
	}

	// save the principal coefficients
	std::vector<jcmp::coefficient> v;
	for(int y = 0; y < height; ++y){
		for(int x = 0; x < width; ++x){
			auto&& el = image[x + width*y];
			if(std::abs(el) > threshold) v.push_back({el, jcmp::uint(x), jcmp::uint(y)});
			else ++zeros;
		}
	}

	return jcmp::image(width, height, std::move(v));
}

std::vector<double> decompress(jcmp::image in){
	auto width = in.header.width;
	auto height = in.header.height;
	assert(is_pow_of_two(width));
	assert(is_pow_of_two(height));

	std::vector<double> image(width * height, 0.0);

	// read in coefficient on coordinates
	for(auto it = in.data.begin(); it != in.data.end(); ++it){
		auto&& x = *it;
		image[x.x + width*x.y] = x.c;
	}

	in.clear();

	// inverse wavelet transform in y-direction
	for(int i = 0; i < width; ++i){
		unwavelet(strided(image.begin() + i, width), strided(image.end() + i, width));
	}

	// inverse wavelet transform in x-direction
	for(int i = 0; i < height; ++i){
		unwavelet(image.begin() + i*width, image.begin() + (i+1)*width);
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
		int zeros = 0;
		auto compressed_vec = decompress(compress(image_vec, width, 0.5, zeros));

		// output some information
		std::cout << field("raw") << human_string(image_vec.size()) << std::endl;
		std::cout << field("compressed") << human_string(image_vec.size() - zeros) << std::endl;

		// save to output file
		std::string cfilename = "compressed/" + path.filename().string();
		png::gray_ostream compressed_image(width, height, cfilename);
		for(int i = 0; i < compressed_vec.size(); ++i) compressed_image << compressed_vec[i];
	}
}
