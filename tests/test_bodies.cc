#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

#include <httpverbs/httpverbs.h>

#include <stdlib.h>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

using namespace httpverbs::keywords;

TEST_CASE("string to string body r/w", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	for (int i = 0; i < 50; ++i)
	{
		auto arr = get_random_block();
		req.data.append(arr.data(), arr.size());
	}

	auto resp = req.perform();

	CHECK(resp.content == req.data);
}

TEST_CASE("callback to callback body r/w", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	sha1 h1, h2;
	size_t nbytes = 100000;

	auto resp = req.perform(
	    nbytes,
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
	    [&](char* s, size_t n) -> size_t
	    {
		h2.update(s, n);

		return n;
	    });

	CHECK(h1.hexdigest() == h2.hexdigest());
}

TEST_CASE("other body policies", "[objects][network]")
{
	SECTION("from data to callback")
	{
		auto req = httpverbs::request("ECHO", host);
		req.data = "Fly me to the moon";

		sha1 h;

		auto resp = req.perform(
		    [&](char* s, size_t n) -> size_t
		    {
			h.update(s, n);

			return n;
		    });

		CHECK(h.hexdigest() == sha1(req.data).hexdigest());
	}

	SECTION("from callback to content")
	{
		auto req = httpverbs::request("ECHO", host);
		char s[] = "And let me play among the stars";

		auto resp = req.perform(
		    sizeof(s) - 1,
		    [&](char* d, size_t n) -> size_t
		    {
			memcpy(d, s, sizeof(s) - 1);

			return sizeof(s) - 1;
		    });

		CHECK(resp.content == s);
	}

	SECTION("from callback to nothing")
	{
		auto req = httpverbs::request("ECHO", host);
		char s[] = "Let me see what spring is like";

		auto resp = req.ignore_response_body().perform(
		    sizeof(s),
		    [&](char* d, size_t n) -> size_t
		    {
			memcpy(d, s, sizeof(s));

			return sizeof(s);
		    });

		REQUIRE(resp.status_code == 200);
		REQUIRE(resp.content.empty());
	}
}
