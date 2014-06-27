#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/exceptions.h>

#include <curl/curl.h>

inline void throw_bad_request()
{
	throw httpverbs::bad_request();
}

TEST_CASE("bad_request")
{
	REQUIRE_THROWS_AS(throw_bad_request(), std::exception);
}

TEST_CASE("bad_response")
{
	try
	{
		throw httpverbs::bad_response(CURLE_OK);
	}
	catch (std::runtime_error& e)
	{
		REQUIRE(std::string(e.what()) == "No error");
	}
}
