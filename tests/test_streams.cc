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

using namespace httpverbs::keywords;

TEST_CASE("C++ streams library support", "[network]")
{
	size_t nbytes = 40000;
	randomstream in(nbytes);
	std::ostringstream out;

	auto req = httpverbs::request("ECHO", host);
	auto resp = req.perform(nbytes, from_stream(in),
	    to_stream(out));

	REQUIRE(out.str().size() == nbytes);
}

TEST_CASE("C stdio library support", "[network][diskio]")
{
	auto file_open = [](char const* fn, char const* mode) -> FILE*
	{
		FILE* in;
#if defined(WIN32)
		if (fopen_s(&in, fn, mode) != 0)
#else
		if ((in = fopen(fn, mode)) == nullptr)
#endif
			FAIL("cannot open file in mode " << mode);

		return in;
	};

	auto sha1_of_file = [](char const* fn) -> std::string
	{
		std::ifstream f(fn, std::ios::binary);
		char buf[16 * 1024];
		sha1 h;

		if (not f.is_open())
			FAIL("cannot open file to calculate sha1");

		while (f.read(buf, sizeof(buf)).good())
			h.update(buf, sizeof(buf));
		h.update(buf, static_cast<size_t>(f.gcount()));

		return h.hexdigest();
	};

	size_t nbytes = 30000;

	{
		randombuf src(nbytes);
		std::ofstream tmp("test_streams_1.tmp", std::ios::binary);

		if (not tmp.is_open())
			FAIL("cannot open temporary file to write");

		tmp << &src;
	}

	defer(std::remove("test_streams_1.tmp"));
	defer(std::remove("test_streams_2.tmp"));

	{
		auto in = file_open("test_streams_1.tmp", "rb");
		defer(fclose(in));

		auto out = file_open("test_streams_2.tmp", "wb");
		defer(fclose(out));

		auto req = httpverbs::request("ECHO", host);
		auto resp = req.perform(nbytes, from_c_file(in),
		    to_c_file(out));
	}

	REQUIRE(sha1_of_file("test_streams_2.tmp") ==
	    sha1_of_file("test_streams_1.tmp"));
}
