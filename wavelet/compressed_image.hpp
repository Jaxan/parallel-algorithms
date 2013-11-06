#pragma once

#include <includes.hpp>

// joshua's compression :D
namespace jcmp {
	typedef uint32_t uint;

	struct __attribute__ ((__packed__)) header {
		const char signature[4];
		uint width;
		uint height;
		uint length;

		header(uint width = 0, uint height = 0, uint length = 0)
		: signature{'J', 'C', 'M', 'P'}
		, width(width)
		, height(height)
		, length(length)
		{}
	};

	struct __attribute__ ((__packed__)) coefficient {
		double c;
		uint x;
		uint y;
	};

	struct image {
		header header;
		std::vector<coefficient> data;

		image() = default;

		image(uint width, uint height, std::vector<coefficient> const & data_in)
		: header(width, height, data_in.size())
		, data(data_in)
		{}

		void clear(){
			header.length = header.height = header.width = 0;
			data.clear();
		}
	};

	inline void write_to_file(image const & image, std::string filename){
		std::filebuf file;
		file.open(filename, std::ios_base::out|std::ios_base::trunc);
		file.sputn(reinterpret_cast<const char*>(&image.header), sizeof(header));
		file.sputn(reinterpret_cast<const char*>(image.data.data()), image.data.size() * sizeof(coefficient));
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

}
