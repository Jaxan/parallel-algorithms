#include <includes.hpp>
#include <utilities.hpp>

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

void test_correctness(){
	const int w = 1024, h = 512;
	std::vector<jcmp::coefficient> v(w*h / 100);
	std::generate(v.begin(), v.end(), gen{w, 0, 0});

	{	// iterator method
		jcmp::write_to_file({w, h}, v.begin(), v.end(), "test.jcmp");
		auto j = jcmp::read_from_file("test.jcmp");

		assert(w == j.header.width);
		assert(h == j.header.height);
		assert(v == j.data);
	}

	{	// copy method
		jcmp::image i(w, h, std::move(v));

		jcmp::write_to_file(i, "test.jcmp");
		auto j = jcmp::read_from_file("test.jcmp");

		assert(w == j.header.width);
		assert(h == j.header.height);
		assert(i.data == j.data);
	}
}

void test_speed(){
	const int w = 4000, h = 8000;
	std::vector<jcmp::coefficient> input(w*h / 20);
	std::generate(input.begin(), input.end(), gen{w, 0, 0});

	for(int i = 0; i < 10; ++i){
		{	timer t("stream");
			jcmp::write_to_file({w, h}, input.begin(), input.end(), "stream.jcmp");
		}

		std::vector<jcmp::coefficient> copy(w*h / 20);
		std::copy(input.begin(), input.end(), copy.begin());

		{	timer t("move");
			jcmp::image i(w, h, std::move(copy));
			jcmp::write_to_file(i, "move.jcmp");
		}

		{	timer t("copy");
			jcmp::image i(w, h, input);
			jcmp::write_to_file(i, "copy.jcmp");
		}
	}
}

int main(){
	srand(time(0));

	test_correctness();
	std::cout << "Test succeeded" << std::endl;

	test_speed();
	std::cout << "Test completed" << std::endl;
}
