#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/header_dict.h>

SCENARIO("header_dict can be copied and moved", "[objects]")
{
	GIVEN("a header_dict with some headers")
	{
		auto hdr = httpverbs::header_dict();

		hdr.add("Q: Sora no kanata ni aru mono wa");
		hdr.add("A: Mayoigo o haha no te ni michibiku mono, EXILE");

		REQUIRE(hdr.size() == 2);

		WHEN("a copy is created")
		{
			auto hdr2 = hdr;

			THEN("they compare to the same")
			{
				REQUIRE(hdr2.size() == hdr.size());
				REQUIRE(hdr2 == hdr);
			}
		}

		WHEN("it is moved into another header_dict")
		{
			auto hdr3 = std::move(hdr);

			THEN("that header_dict replaces the given one")
			{
				REQUIRE(hdr3.size() == 2);
				REQUIRE(hdr3.get("Q"));
				REQUIRE(hdr3.get("A"));

				// XXX illegal test
				REQUIRE(hdr.empty());
			}
		}
	}
}

TEST_CASE("header_dict non-header handling", "[objects]")
{
	auto hdr = httpverbs::header_dict();

	SECTION("header without colon (libcurl)")
	{
		hdr.add("not-a-header;");

		REQUIRE(hdr.empty());
		REQUIRE_FALSE(hdr.get("not-a-header"));
		REQUIRE_THROWS(hdr["not-a-header"]);
	}
}

SCENARIO("duplicated field-name lookup", "[objects]")
{
	GIVEN("a header_dict with sparse headers with same name")
	{
		auto hdr = httpverbs::header_dict();

		hdr.add("multi: ignored");
		hdr.add("multi-header: 2");
		hdr.add("multi-header: 1");
		hdr.add("multi-header-after: ignored");
		hdr.add("multi-header: 3");

		REQUIRE(hdr.size() == 5);

		THEN("you get those headers joined in insertion order")
		{
			auto s = hdr.get("multi-header");

			REQUIRE(s);
			REQUIRE(hdr["multi-header"] == s);
			REQUIRE(hdr["multi-header"] == "2, 1, 3");
		}

		WHEN("more headers appended")
		{
			hdr.add("multi-header:4,5");
			hdr.add("multi-header:  6  7  ");

			REQUIRE(hdr.size() == 7);

			THEN("you get all those headers joined")
			{
				REQUIRE(hdr["multi-header"] ==
				    "2, 1, 3, 4,5, 6  7");
			}
		}
	}
}
