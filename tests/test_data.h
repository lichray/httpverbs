#include <random>
#include <array>
#include <algorithm>
#include <iterator>
#include <string>

static auto e = std::mt19937(std::random_device()());

inline
auto get_random_block() -> std::array<char, 4096>
{
	std::array<char, 4096> arr;

	auto it = reinterpret_cast<decltype(e())*>(arr.data());
	auto n = arr.size() / sizeof(decltype(e()));

#if !defined(_MSC_VER)
	std::generate_n(it, n, e);
#else
	std::generate_n(stdext::make_unchecked_array_iterator(it), n, e);
#endif

	return arr;
}

template <typename ForwardIt, typename OutputIt, typename Size, class URNG>
inline
OutputIt sample(ForwardIt first, ForwardIt last, OutputIt d_first,
    Size n, URNG&& g)
{
	typedef std::uniform_int_distribution<Size>	dist_t;
	typedef typename dist_t::param_type		param_t;

	static dist_t d;

	Size unsampled = std::distance(first, last);

	for (n = std::min(n, unsampled); n != 0; ++first)
	{
		if (d(g, param_t(0, --unsampled)) < n)
		{
			*d_first++ = *first;
			--n;
		}
	}

	return d_first;
}

template <typename IntType>
inline
IntType randint(IntType a, IntType b)
{
	typedef std::uniform_int_distribution<IntType>	dist_t;
	typedef typename dist_t::param_type		param_t;

	static dist_t d;

	return d(e, param_t(a, b));
}

inline
auto get_random_text(size_t len, std::string const& from =
    "-0123456789abcdefghijklmnopqrstuvwxyz")
	-> std::string
{
	std::string to;
	sample(begin(from), end(from), std::back_inserter(to), len, e);

	return to;
}
