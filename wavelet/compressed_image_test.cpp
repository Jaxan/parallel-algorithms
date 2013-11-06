#include <includes.hpp>

#include "compressed_image.hpp"
namespace jcmp{
	bool operator==(coefficient const & lhs, coefficient const & rhs){
		return lhs.c == rhs.c && lhs.x == rhs.x && lhs.y == rhs.y;
	}
}

struct gen{
	jcmp::uint width, x, y;

	jcmp::coefficient operator()(){
		++x;
		if(x > width){
			x = 0;
			++y;
		}
		// only for testing purpose :D
		return {rand() / double(RAND_MAX), x, y};
	}
};

int main(){
	const int w = 1024, h = 512;
	srand(time(0));

	std::vector<jcmp::coefficient> v(w*h / 100);
	std::generate(v.begin(), v.end(), gen{w, 0, 0});

	jcmp::image i(w, h, std::move(v));

	jcmp::write_to_file(i, "test.jcmp");
	auto j = jcmp::read_from_file("test.jcmp");

	assert(w == j.header.width);
	assert(h == j.header.height);
	assert(i.data.size() == j.data.size());
	assert(i.data == j.data);

	std::cout << "Test succeeded" << std::endl;
}
