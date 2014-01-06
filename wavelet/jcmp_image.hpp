#pragma once

#include <includes.hpp>

#include "jcmp_quantization.hpp"

// joshua's compression :D
namespace jcmp {
	typedef uint32_t uint;

	struct header {
		char signature[4];
		uint width;
		uint height;
		uint length;
		quantize_params qp;

		header() = default;
		header(uint width_, uint height_, uint length_, quantize_params const & p)
		: signature{'J', 'C', 'M', 'P'}
		, width(width_)
		, height(height_)
		, length(length_)
		, qp(p)
		{}

		template <typename F>
		quantization get_quantization(F const & f, F const & f_inv){
			quantization q;
			q.p = qp;
			q.f = f;
			q.f_inv = f_inv;
			return q;
		}
	};

	struct __attribute__ ((__packed__)) coefficient {
		uint8_t c;
		uint x;
		uint y;
	};

	struct image {
		header header;
		std::vector<coefficient> data;

		image() = default;

		image(uint width, uint height, quantize_params const & p, std::vector<coefficient> const & data_in)
		: header(width, height, data_in.size(), p)
		, data(data_in)
		{}

		void clear(){
			header.length = header.height = header.width = 0;
			header.qp.f_max_abs = header.qp.f_min_abs = 0;
			data.clear();
		}
	};

	// Way of writing does not matter for performance
	// timings: http://i.imgur.com/cskC43O.png (300 points per method)
	// It's not surprising as filebuf is already a buffer
	inline void write_to_file(image const & image, std::string filename){
		std::filebuf file;
		file.open(filename, std::ios_base::out|std::ios_base::trunc);
		file.sputn(reinterpret_cast<const char*>(&image.header), sizeof(header));
		file.sputn(reinterpret_cast<const char*>(image.data.data()), image.data.size() * sizeof(coefficient));
	}

	// Assumes Iterator points to a coefficient]
	// length in header is not used
	template <typename Iterator>
	inline void write_to_file(header h, Iterator begin, Iterator end, std::string filename){
		h.length = 0;
		std::filebuf file;
		file.open(filename, std::ios_base::out|std::ios_base::trunc);
		file.sputn(reinterpret_cast<const char*>(&h), sizeof(header));
		while(begin != end){
			file.sputn(reinterpret_cast<const char*>(&*begin++), sizeof(coefficient));
			h.length++;
		}
		// http://stackoverflow.com/questions/2530274/how-to-overwrite-only-part-of-a-file-in-c
		file.pubseekpos(0);
		file.sputn(reinterpret_cast<const char*>(&h), sizeof(header));
	}

	inline image read_from_file(std::string const& filename){
		std::filebuf file;
		file.open(filename, std::ios_base::in);
		image image;
		file.sgetn(reinterpret_cast<char*>(&image.header), sizeof(header));
		assert(strncmp("JCMP", image.header.signature, 4) == 0);
		image.data.resize(image.header.length);
		file.sgetn(reinterpret_cast<char*>(image.data.data()), image.data.size() * sizeof(coefficient));
		return image;
	}

	static_assert(sizeof(header) == 32,     "struct not propery packed");
	static_assert(sizeof(coefficient) == 9, "struct not propery packed");
}
