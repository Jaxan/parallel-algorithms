#include <includes.hpp>
#include <utilities.hpp>

#include "jcmp_image.hpp"

namespace jcmp{
	inline bool operator==(coefficient const & lhs, coefficient const & rhs){
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
		return {uint8_t(rand()), x, y};
	}
};

static void test_correctness(){
	const int w = 1024, h = 512;
	std::vector<jcmp::coefficient> v(w*h / 100);
	std::generate(v.begin(), v.end(), gen{w, 0, 0});

	{	// iterator method
		jcmp::write_to_file({w, h, 0, {0.0, 0.0}}, v.begin(), v.end(), "test.jcmp");
		auto j = jcmp::read_from_file("test.jcmp");

		assert(w == j.header.width);
		assert(h == j.header.height);
		assert(v == j.data);
	}

	{	// copy method
		jcmp::image i(w, h, {0.0, 0.0}, std::move(v));

		jcmp::write_to_file(i, "test.jcmp");
		auto j = jcmp::read_from_file("test.jcmp");

		assert(w == j.header.width);
		assert(h == j.header.height);
		assert(i.data == j.data);
	}
}

static void test_speed(){
	const int w = 4000, h = 8000;
	std::vector<jcmp::coefficient> input(w*h / 20);
	std::generate(input.begin(), input.end(), gen{w, 0, 0});

	for(int i = 0; i < 10; ++i){
		{	timer t("stream");
			jcmp::write_to_file({w, h, 0, {0.0, 0.0}}, input.begin(), input.end(), "stream.jcmp");
		}

		std::vector<jcmp::coefficient> copy(w*h / 20);
		std::copy(input.begin(), input.end(), copy.begin());

		{	timer t("move");
			jcmp::image j(w, h, {0.0, 0.0}, std::move(copy));
			jcmp::write_to_file(j, "move.jcmp");
		}

		{	timer t("copy");
			jcmp::image j(w, h, {0.0, 0.0}, input);
			jcmp::write_to_file(j, "copy.jcmp");
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
