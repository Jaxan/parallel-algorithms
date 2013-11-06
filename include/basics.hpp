//
//  basics.hpp
//  ImageStreams
//
//  Created by Joshua Moerman on 9/7/12.
//  Copyright (c) 2012 Vadovas. All rights reserved.
//

#ifndef ImageStreams_basics_hpp
#define ImageStreams_basics_hpp

/*
 A PixelFormat p should have the following constructors (depending on number
 of colors):
	p(int r, int g, int b, int a)
	p(double r, double g, double b, double a)
	
	p(int v)
	p(double v)
 
 The order in the ctor is rgb even though the internal structure may be
 different
 
 It should be trivially copyable (for memcpy).
 
 Furthermore it should have a static const size_t num_colors indicating the
 number of colors and a static const size_t bit_per_color indicating the
 number of bits per color (TODO: what to do with encodings like 565 ?)
 */

#include <cstdint>
#include <algorithm>

namespace pixel_formats {
	inline uint8_t clamp(int n){
		return std::min(255, std::max(0, n));
	}
	
	struct gray {
		static const size_t num_colors = 1;
		static const size_t bits_per_color = 8;
		
		gray() : value(0) {}
		gray(double intensity) : value(clamp(255*intensity)) {}
		gray(int intensity) : value(clamp(intensity)) {}
		
	private:
		uint8_t value;
	};
	
	struct rgb {
		static const size_t num_colors = 3;
		static const size_t bits_per_color = 8;
		
		rgb()
		: red(0)
		, green(0)
		, blue(0)
		{}
		
		rgb(double red, double green, double blue)
		: red(clamp(255*red))
		, green(clamp(255*green))
		, blue(clamp(255*blue))
		{}
		
		rgb(int red, int green, int blue)
		: red(clamp(red))
		, green(clamp(green))
		, blue(clamp(blue))
		{}
		
	private:
		uint8_t red;
		uint8_t green;
		uint8_t blue;
	};
	
	struct bgr{
		static const size_t num_colors = 3;
		static const size_t bits_per_color = 8;
		
		bgr()
		: blue(0)
		, green(0)
		, red(0)
		{}
		
		bgr(double red, double green, double blue)
		: blue(clamp(255*blue))
		, green(clamp(255*green))
		, red(clamp(255*red))
		{}
		
		bgr(int red, int green, int blue)
		: blue(clamp(blue))
		, green(clamp(green))
		, red(clamp(red))
		{}
		
	private:
		uint8_t blue;
		uint8_t green;
		uint8_t red;
	};

	template <typename PixelType>
	struct traits {
		static const size_t num_colors = PixelType::num_colors;
		static const size_t bits_per_color = PixelType::bits_per_color;
		static const size_t bits_per_pixel = num_colors * bits_per_color;
	};
}

#endif
