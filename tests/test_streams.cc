#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_data.h"

#include <httpverbs/httpverbs.h>
#include <httpverbs/stream.h>
#include <httpverbs/c_file.h>

#include <sstream>
#include <fstream>

#include "../src/stdex/defer.h"

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

using httpverbs::from_stream;
using httpverbs::to_stream;
using httpverbs::from_c_file;
using httpverbs::to_c_file;
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

TEST_CASE("C stdio library support", "[network][diskio]")
{
	auto sha1_of_file = [](char const* fn) -> std::string
	{
		std::ifstream f(fn, std::ios::binary);
		char buf[16 * 1024];
		sha1 h;

		if (not f.is_open())
			FAIL("cannot open file to calculate sha1");

		while (f.read(buf, sizeof(buf)))
			h.update(buf, sizeof(buf));
		h.update(buf, f.gcount());

		return h.hexdigest();
	};

	size_t nbytes = 30000;

	{
		randombuf src(nbytes);
		std::ofstream tmp("test_streams_1.tmp");

		if (not tmp.is_open())
			FAIL("cannot open temporary file to write");

		tmp << &src;
	}

	defer(std::remove("test_streams_1.tmp"));
	defer(std::remove("test_streams_2.tmp"));

	{
		auto in = fopen("test_streams_1.tmp", "rb");

		if (in == nullptr)
			FAIL("cannot open temporary file to read");

		defer(fclose(in));

		auto out = fopen("test_streams_2.tmp", "wb");

		if (in == nullptr)
			FAIL("cannot open temporary file to read");

		defer(fclose(out));

		auto req = httpverbs::request("ECHO", host);
		auto resp = req.perform(from_c_file(in), of_length(nbytes),
		    to_c_file(out));
	}

	REQUIRE(sha1_of_file("test_streams_2.tmp") ==
	    sha1_of_file("test_streams_1.tmp"));
}
