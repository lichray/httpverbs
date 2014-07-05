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

	auto be = reinterpret_cast<std::mt19937::result_type*>(&*begin(arr));
	auto ed = reinterpret_cast<std::mt19937::result_type*>(&*end(arr));

	std::generate(be, ed, e);

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

inline
auto get_random_text(size_t maxlen, std::string const& from =
    "-0123456789abcdefghijklmnopqrstuvwxyz")
	-> std::string
{
	typedef std::uniform_int_distribution<size_t>	dist_t;
	typedef typename dist_t::param_type		param_t;

	static dist_t d;

	std::string to;

	auto l = d(e, param_t(1, maxlen));
	sample(begin(from), end(from), std::back_inserter(to), l, e);

	return to;
}
