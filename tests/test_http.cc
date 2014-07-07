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

			THEN("the url is no different from the request")
			{
				REQUIRE(resp.url == req.url);
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

TEST_CASE("response receiving headers", "[objects][network]")
{
	auto resp = httpverbs::request("OPTIONS", host).perform();

	REQUIRE(resp.status_code == 200);

	SECTION("header not presented")
	{
		auto s = resp.get_header("content-type");

		REQUIRE_FALSE(s);
	}

	SECTION("headers presented")
	{
		auto s1 = resp.get_header("server");
		auto s2 = resp.get_header("date");

		REQUIRE(s1);
		REQUIRE_FALSE(s1.get().empty());

		REQUIRE(s2);
		REQUIRE_FALSE(s2.get().empty());
	}

	SECTION("header is case-insensitive")
	{
		auto s1 = resp.get_header("Allow");
		auto s2 = resp.get_header("allow");
		auto s3 = resp.get_header("aLLoW");

		REQUIRE(s1 == std::string("GET, PUT, DELETE"));
		REQUIRE(s2 == s1);
		REQUIRE(s3 == s2);
	}
}

SCENARIO("request can send and receive body", "[objects][network]")
{
	auto k3 = host + "k3";
	auto body = std::string("data with \0 inside", 18);

	GIVEN("a request with some data")
	{
		auto req = httpverbs::request("PUT", k3);
		req.data = body;

		WHEN("the key is created")
		{
			auto resp = req.perform();

			REQUIRE(resp.status_code == 201);

			THEN("the value of the key is the given data")
			{
				req = httpverbs::request("GET", k3);
				resp = req.perform();

				REQUIRE(resp.status_code == 200);
				REQUIRE(resp.content == body);
			}
		}
	}

	GIVEN("a request with a method expecting body")
	{
		using httpverbs::from_data;
		using httpverbs::ignoring_response_body;

		auto req = httpverbs::request("GET", k3);

		WHEN("the query performed by ignoring body")
		{
			auto resp = req.perform(from_data,
			    ignoring_response_body);

			REQUIRE(resp.status_code == 200);

			THEN("response is received but body is discarded")
			{
				REQUIRE(resp.get_header("content-type"));
				REQUIRE(resp.content.empty());
			}
		}
	}
}
