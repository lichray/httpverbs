#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

#include "test_data.h"

#include <chrono>
#include <iostream>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

TEST_CASE("string to string body r/w", "[network][mass]")
{
	SECTION("small amount")
	{
		auto req = httpverbs::request("ECHO", host);
		req.data = "fly me to the moon";
		auto resp = req.perform();

		CHECK(resp.content == req.data);
	}

	SECTION("large amount")
	{
		auto req = httpverbs::request("ECHO", host);

		for (int i = 0; i < 1024; ++i)
		{
			auto arr = get_random_block();
			req.data.append(reinterpret_cast<char const*>(
			    arr.data()), arr.size() * sizeof(arr[0]));
		}

		auto resp = req.perform();

		CHECK(resp.content == req.data);
	}
}
