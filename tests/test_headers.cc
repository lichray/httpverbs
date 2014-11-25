#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

#include <httpverbs/httpverbs.h>
#include <boost/optional/optional_io.hpp>

#include <curl/curlver.h>

#include <unordered_map>
#include <utility>
#include <tuple>
#include <stdlib.h>
#include <stdio.h>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

TEST_CASE("value has no surrounding whitespace", "[objects][network]")
{
	auto req = httpverbs::request("ECHO", host);

	req.headers.add("X-EVA-01", " \t\t\t purple");
	req.headers.add("X-EVA-00", "blue\t\t\t");
	req.headers.add("X-EVA-02", " red\t");

	auto resp = req.perform();

	CHECK(resp.headers["X-EVA-01"] == "purple");
	CHECK(resp.headers["X-EVA-00"] == "blue");
	CHECK(resp.headers["X-EVA-02"] == "red");
}

TEST_CASE("can retrieve empty field-content", "[objects][network]")
{
	auto req = httpverbs::request("ECHO", host);

	// double evil hack to test empty header support
	req.headers.add("X-ignore:\r\nX-NERV", " ");

	auto resp = req.perform();
	auto h = resp.headers.get("X-NERV");

	REQUIRE(h);
	REQUIRE(h.get().empty());
}

TEST_CASE("massive unique header lookup", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	std::unordered_map<std::string, int> m;

	for (int i = 1; i < 40; ++i)
	{
		bool inserted;
		decltype(begin(m)) it;

		std::tie(it, inserted) = m.insert(std::make_pair(
		    "x-" + get_random_text(randint(1, 10)), i));

		if (inserted)
		{
			char ibuf[10];
#if defined(WIN32)
			sprintf_s(ibuf, sizeof(ibuf), "%d", i);
#else
			snprintf(ibuf, sizeof(ibuf), "%d", i);
#endif

			req.headers.add(it->first, ibuf);
		}
	}

	auto resp = req.perform();

	for (auto it = begin(m); it != end(m); ++it)
	{
		auto h = resp.headers[it->first];

		CHECK(strtol(h.data(), NULL, 10) == it->second);
	}
}

TEST_CASE("long header insertion and lookup", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	SECTION("longer than local buffer")
	{
		auto h = get_random_text(150);
		req.headers.add("X-I-1", h);
		auto resp = req.perform();

		REQUIRE(resp.headers.get("X-I-1") == h);
	}

	SECTION("reallocate triggered")
	{
		auto h = get_random_text(20000);
		req.headers.add("X-WUG", h);
		auto resp = req.perform();

		REQUIRE(resp.headers.get("X-WUG") == h);
	}
}
