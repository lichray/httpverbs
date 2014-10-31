#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

httpverbs::enable_library _;

TEST_CASE("customized CA bundle path", "[network]")
{
	httpverbs::enable_library e1("nonexistent");

	REQUIRE_THROWS(httpverbs::get("https://httpbin.org/get"));

	// disable the test because CA bundle is usually not
	// available on windows
#if !defined(WIN32)
	httpverbs::enable_library e2;

	REQUIRE_NOTHROW(httpverbs::get("https://httpbin.org/get"));
#endif
}
