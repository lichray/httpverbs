#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

#include <curl/curl.h>

TEST_CASE("enable_library")
{
	httpverbs::enable_library _;

	auto handle = curl_easy_init();
	CHECK(handle);

	curl_easy_cleanup(handle);
}
