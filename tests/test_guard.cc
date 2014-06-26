#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

#include <curl/curl.h>

TEST_CASE("guard")
{
	httpverbs::guard _;

	auto handle = curl_easy_init();
	CHECK(handle);

	curl_easy_cleanup(handle);
}
