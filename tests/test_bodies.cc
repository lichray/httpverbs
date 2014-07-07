#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

#include <httpverbs/httpverbs.h>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

using httpverbs::from_data;
using httpverbs::to_content;
using httpverbs::of_length;

TEST_CASE("string to string body r/w", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	for (int i = 0; i < 50; ++i)
	{
		auto arr = get_random_block();
		req.data.append(arr.data(), arr.size());
	}

	auto resp = req.perform(from_data, to_content);

	CHECK(resp.content == req.data);
}

TEST_CASE("callback to callback body r/w", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	sha1 h1, h2;
	size_t nbytes = 100000;

	auto resp = req.perform(
	    [&, nbytes](char* d, size_t n) mutable -> size_t
	    {
		if (nbytes < n)
			n = nbytes;

		auto s = get_random_text(n);

#if !defined(_MSC_VER)
		s.copy(d, n);
#else
		std::copy(begin(s), end(s),
		    stdext::make_unchecked_array_iterator(d));
#endif
		nbytes -= n;
		h1.update(s);

		return n;
	    },
	    of_length(nbytes),
	    [&](char* s, size_t n) -> size_t
	    {
		h2.update(s, n);

		return n;
	    });

	CHECK(h1.hexdigest() == h2.hexdigest());
}
