#include <random>
#include <array>
#include <algorithm>

inline
auto get_random_block()
	-> std::array
	<
	    std::mt19937::result_type,
	    4096 / sizeof(std::mt19937::result_type)
	>
{
	static auto e = std::mt19937(std::random_device()());

	std::array
	<
	    std::mt19937::result_type,
	    4096 / sizeof(std::mt19937::result_type)
	>
	arr;

	std::generate(begin(arr), end(arr), e);

	return arr;
}
