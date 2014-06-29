#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

SCENARIO("request can perform simple queries", "[objects][network]")
{
	auto k1 = host + "k1";

	GIVEN("a request on a nonexistent key")
	{
		auto req = httpverbs::request("GET", k1);

		WHEN("the query performed")
		{
			auto resp = req.perform();

			THEN("the status is set to not found")
			{
				REQUIRE(resp.status_code == 404);
				REQUIRE_FALSE(resp.ok());
			}
		}

		WHEN("the key is touched")
		{
			auto req = httpverbs::request("PUT", k1);
			auto resp = req.perform();

			REQUIRE(resp.status_code == 201);
			REQUIRE(resp.ok());

			THEN("the key can be get")
			{
				req = httpverbs::request("GET", k1);
				resp = req.perform();

				REQUIRE(resp.status_code == 200);
				REQUIRE(resp.ok());
			}
		}
	}
}

TEST_CASE("request with customized headers", "[objects][network]")
{
	auto k2 = host + "k2";

	SECTION("send unaccepted content-type")
	{
		auto req = httpverbs::request("PUT", k2);
		req.add_header("Content-Type", "text/html");

		auto resp = req.perform();

		REQUIRE(resp.status_code == 400);
	}

	SECTION("send accepted content-type")
	{
		auto req = httpverbs::request("PUT", k2);
		req.add_header("Content-Type", "text/plain");

		auto resp = req.perform();

		REQUIRE(resp.status_code == 201);
	}
}
