#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

#include <httpverbs/httpverbs.h>

#include <chrono>
#include <iostream>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

TEST_CASE("string to string body r/w", "[network][mass]")
{
	auto req = httpverbs::request("ECHO", host);

	for (int i = 0; i < 500; ++i)
	{
		auto arr = get_random_block();
		req.data.append(reinterpret_cast<char const*>(
		    arr.data()), arr.size() * sizeof(arr[0]));
	}

	auto resp = req.perform();

	CHECK(resp.content == req.data);
}
