#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/header_dict.h>

SCENARIO("header_dict can be copied and moved", "[objects]")
{
	GIVEN("a header_dict with some headers")
	{
		auto hdr = httpverbs::header_dict();

		hdr.add_line("Q: Sora no kanata ni aru mono wa");
		hdr.add("A", "Mayoigo o haha no te ni michibiku mono, EXILE");

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
		hdr.add_line("not-a-header;");

		REQUIRE(hdr.empty());
		REQUIRE_FALSE(hdr.get("not-a-header"));
	}
}
