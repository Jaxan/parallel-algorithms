//
//  png.hpp
//  ImageStreams
//
//  Created by Joshua Moerman on 9/7/12.
//  Copyright (c) 2012 Vadovas. All rights reserved.
//

#ifndef ImageStreams_png_hpp
#define ImageStreams_png_hpp

#include <cstdio>
#include <stdexcept>
#include <vector>
#include <png.h>
#include "basics.hpp"

namespace png{
	template <typename P>
	int color_type(){
		int num_colors = pixel_formats::traits<P>::num_colors;
		switch (num_colors) {
			case 1: return PNG_COLOR_TYPE_GRAY;
			case 2: return PNG_COLOR_TYPE_GRAY_ALPHA;
			case 3: return PNG_COLOR_TYPE_RGB;
			case 4: return PNG_COLOR_TYPE_RGB_ALPHA;
			default: return PNG_COLOR_TYPE_RGB;
		}
	}
	
	template <typename P = pixel_formats::rgb>
	struct ostream{
		typedef P pixel;
		
		ostream() = delete;
		ostream(ostream const &) = delete;
		ostream & operator=(ostream const &) = delete;
		
		ostream(uint32_t width, uint32_t height, std::string filename)
		: fp(0)
		, png_ptr(0)
		, info_ptr(0)
		, row(width)
		, x(0)
		{
			fp = fopen(filename.c_str(), "wb");
			if(!fp) throw std::runtime_error("Could not open file");
			
			png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
			if(!png_ptr) throw std::runtime_error("PNG structure could not be allocated");
			
			info_ptr = png_create_info_struct(png_ptr);
			if(!info_ptr) throw std::runtime_error("PNG information structure could not be allocated");
			
			png_init_io(png_ptr, fp);
			
			png_set_IHDR(png_ptr, info_ptr, width, height, pixel_formats::traits<pixel>::bits_per_color, color_type<pixel>(), PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			
			png_set_compression_level(png_ptr, 9);
			
			png_write_info(png_ptr, info_ptr);
		}
		
		~ostream(){
			// NOTE: the pnglib already checks for us whether all pixels are written
			png_write_end(png_ptr, info_ptr);
			png_destroy_info_struct(png_ptr, &info_ptr);
			fclose(fp);
		}
		
		ostream& operator<<(pixel const & p){
			row[x] = p;
			++x;
			if(x >= row.size()){
				png_write_row(png_ptr, reinterpret_cast<unsigned char const*>(row.data()));
				x = 0;
			}
			
			return *this;
		}
		
	private:
		FILE* fp;
		png_structp png_ptr;
		png_infop info_ptr;
		
		std::vector<pixel> row;
		uint32_t x;
	};
	
	typedef ostream<> colored_ostream;
	typedef ostream<pixel_formats::gray> gray_ostream;
	
	struct istream{
		//typedef pixel pixel;
		
		istream(std::string filename)
		: fp(0)
		, png_ptr(0)
		, info_ptr(0)
		, valid(true)
		, row()
		, stride(0)
		, x(0)
		, y(0)
		, height(0)
		{
			fp = fopen(filename.c_str(), "rb");
			if(!fp) throw std::runtime_error(filename + " could not be opened");
			
			png_byte header[8];
			fread(header, 1, 8, fp);
			
			if(png_sig_cmp(header, 0, 8) != 0) throw std::runtime_error("File is not a PNG file.");
			
			png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
			if(!png_ptr) throw std::runtime_error("PNG structure could not be allocated");
			
			info_ptr = png_create_info_struct(png_ptr);
			if(!info_ptr) throw std::runtime_error("PNG information structure could not be allocated");
			
			png_init_io(png_ptr, fp);
			
			png_set_sig_bytes(png_ptr, 8);
			
			png_read_info(png_ptr, info_ptr);
			
			if(png_get_interlace_type(png_ptr, info_ptr) != PNG_INTERLACE_NONE)
				throw std::runtime_error("Interalced PNG's are not supported");
			
			height = png_get_image_height(png_ptr, info_ptr);
			auto width = png_get_image_width(png_ptr, info_ptr);
			auto bit_depth = png_get_bit_depth(png_ptr, info_ptr);
			auto channels = png_get_channels(png_ptr, info_ptr);
			
			// number of bytes in one row
			stride = bit_depth * channels / 8;
			size_t row_size = width * stride;
			
			if(bit_depth < 8) throw std::runtime_error("Bitdepths lower than 8 are not supported (yet)");
			
			row.resize(row_size);
			png_read_row(png_ptr, (png_bytep)row.data(), 0);
		}
		
		~istream(){
			png_read_end(png_ptr, 0);
			png_destroy_read_struct(&png_ptr, &info_ptr, 0);
			fclose(fp);
		}
		
		template <typename Pixel>
		istream& operator>>(Pixel & p){
			if( y >= height){
				valid = false;
				return *this;
			}
			p = row[x];
			
			if(++x >= get_width()){
				x = 0;
				++y;
				if(y < height)
					png_read_row(png_ptr, (png_bytep)row.data(), 0);
			}
			
			return *this;
		}
		
		operator bool() const {
			return valid;
		}
		
		uint32_t get_width() const { return  row.size(); }
		uint32_t get_height() const { return height; }
		
	private:
		FILE* fp;
		png_structp png_ptr;
		png_infop info_ptr;
		bool valid;
		
		std::vector<char> row;
		uint32_t stride;
		uint32_t x;
		uint32_t y;
		uint32_t height;
	};
}

#endif
