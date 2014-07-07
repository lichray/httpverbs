#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

#include <httpverbs/httpverbs.h>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

TEST_CASE("string to string body r/w", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	for (int i = 0; i < 500; ++i)
	{
		auto arr = get_random_block();
		req.data.append(arr.data(), arr.size());
	}

	auto resp = req.perform();

	CHECK(resp.content == req.data);
}
