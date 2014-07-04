#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

#include <curl/curlver.h>

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
