#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

#include <httpverbs/httpverbs.h>
#include <httpverbs/stream.h>

#include <sstream>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

using httpverbs::from_stream;
using httpverbs::to_stream;
using httpverbs::of_length;

TEST_CASE("C++ streams library support", "[network]")
{
	size_t nbytes = 40000;
	randomstream in(nbytes);
	std::ostringstream out;

	auto req = httpverbs::request("ECHO", host);
	auto resp = req.perform(from_stream(in), of_length(nbytes),
	    to_stream(out));

	REQUIRE(out.str().size() == nbytes);
}
