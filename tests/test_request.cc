#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

#include <curl/curl.h>

httpverbs::enable_library _;
char url[] = "http://localhost/";

SCENARIO("request can be moved", "[objects]")
{
	GIVEN("a request")
	{
		auto req = httpverbs::request("GET", url);

		REQUIRE(req.url == url);

		WHEN("it is moved into another request")
		{
			auto req2 = std::move(req);

			THEN("that request replaces the given one")
			{
				REQUIRE(req2.url == url);
				REQUIRE(req.url.empty());
			}
		}

		WHEN("it is moved from another request")
		{
			req = httpverbs::request("DELETE", "127.0.0.1");

			THEN("it becomes that request")
			{
				REQUIRE(req.url == "127.0.0.1");
			}
		}
	}
}

TEST_CASE("request comparison", "[objects]")
{
	auto req1 = httpverbs::request("GET", url);
	auto req2 = httpverbs::request("GET", url);

	REQUIRE(req1 == req1);
	REQUIRE(req2 == req2);
	REQUIRE(req1 != req2);
}

TEST_CASE("of_length as a safe curl_off_t carrier", "[objects]")
{
	using httpverbs::of_length;

	CHECK(curl_off_t(of_length(-1)) == CURL_OFF_T_C(-1));
	CHECK(curl_off_t(of_length(2147483647L)) == CURL_OFF_T_C(2147483647));
	CHECK(curl_off_t(of_length(size_t(2147483648UL))) ==
	    CURL_OFF_T_C(2147483648));

	if (sizeof(size_t) == sizeof(curl_off_t))
		CHECK_THROWS(of_length(size_t(-1)));
}
