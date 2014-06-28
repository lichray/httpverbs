#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

httpverbs::enable_library _;

SCENARIO("response can be copied and moved", "[objects]")
{
	GIVEN("a response with a url")
	{
		auto resp = httpverbs::response(200);
		resp.url = "http://localhost/";

		REQUIRE(resp.status_code == 200);
		REQUIRE(resp == resp);
		REQUIRE(resp != httpverbs::response(200));

		WHEN("a copy is created")
		{
			auto resp2 = resp;

			THEN("they compare to the same")
			{
				REQUIRE(resp2.status_code == 200);
				REQUIRE(resp2 == resp);
			}
		}

		WHEN("it is moved into another response")
		{
			auto resp3 = std::move(resp);

			THEN("that response replaces the given one")
			{
				REQUIRE(resp3.status_code == 200);
				REQUIRE(resp3.url == "http://localhost/");
				REQUIRE(resp.url.empty());
			}
		}
	}
}

TEST_CASE("response status", "[objects]")
{
	CHECK(httpverbs::response(100).ok());
	CHECK(httpverbs::response(200).ok());
	CHECK_FALSE(httpverbs::response(400).ok());
	CHECK_FALSE(httpverbs::response(500).ok());
	CHECK(httpverbs::response(600).ok());
}
