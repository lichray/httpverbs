#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

#include <httpverbs/httpverbs.h>

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

	req.add_header("X-EVA-01", " \t\t\t purple");
	req.add_header("X-EVA-00", "blue\t\t\t");
	req.add_header("X-EVA-02", " red\t");

	auto resp = req.perform();

	CHECK(resp.get_header("X-EVA-01") == std::string("purple"));
	CHECK(resp.get_header("X-EVA-00") == std::string("blue"));
	CHECK(resp.get_header("X-EVA-02") == std::string("red"));
}

TEST_CASE("field-content can be empty", "[objects][network]")
{
	auto req = httpverbs::request("ECHO", host);

#if LIBCURL_VERSION_NUM >= 0x071700
	req.add_header("X-NERV", "");
#else
	// double evil hack to test empty header support
	req.add_header("X-ignore:\r\nX-NERV", " ");
#endif

	auto resp = req.perform();

	REQUIRE(resp.get_header("X-NERV"));
	REQUIRE(resp.get_header("X-NERV").get().empty());
}

TEST_CASE("massive unique header lookup", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	std::unordered_map<std::string, int> m;
	std::string buf = "x-";

	for (int i = 1; i < 40; ++i)
	{
		bool inserted;
		decltype(begin(m)) it;

		append_random_text(buf, 10);
		std::tie(it, inserted) = m.insert(std::make_pair(buf, i));
		buf.erase(2);

		if (inserted)
		{
			char ibuf[10];
			snprintf(ibuf, sizeof(ibuf), "%d", i);

			req.add_header(it->first, ibuf);
		}
	}

	auto resp = req.perform();

	for (auto it = begin(m); it != end(m); ++it)
	{
		auto h = resp.get_header(it->first);

		REQUIRE(h);
		CHECK(strtol(h.get().data(), NULL, 10) == it->second);
	}
}
