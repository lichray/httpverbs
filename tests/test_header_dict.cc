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

SCENARIO("header_dict can handle multi-header", "[objects]")
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

		WHEN("you set the name to a value")
		{
			hdr.set("multi-header", "4, 2");

			THEN("all headers with that name are replaced")
			{
				REQUIRE(hdr.size() == 3);
				REQUIRE(hdr["multi-header"] == "4, 2");
			}
		}

		WHEN("you remove the name")
		{
			auto n = hdr.erase("multi-header");

			REQUIRE(n == 3);

			THEN("all headers with that name gone")
			{
				REQUIRE_FALSE(hdr.get("multi-header"));
				REQUIRE(hdr.size() == 2);
			}
		}
	}
}

TEST_CASE("header_dict edge cases", "[objects]")
{
	auto hdr = httpverbs::header_dict();

	SECTION("removing non-existing header")
	{
		auto n = hdr.erase("Q");

		REQUIRE(n == 0);
		REQUIRE(hdr.empty());
	}

	SECTION("setting non-existing header")
	{
		hdr.set("Q", "Daichi o kin'iro ni someru nami");

		REQUIRE(hdr.get("Q"));
		REQUIRE(hdr.size() == 1);
	}

	SECTION("header without space after colon")
	{
		std::string s1 = "Inochi o hagukumu Megumi no ibuki";
		std::string s2 = "mugi no daichi";

		hdr.add("A:" + s1);

		REQUIRE(*begin(hdr) == "A:" + s1);
		REQUIRE(hdr["A"] == s1);

		hdr.set("a", s2);

		REQUIRE(*begin(hdr) == "A: " + s2);
		REQUIRE(hdr["A"] == s2);
	}

	SECTION("header without colon is ignored")
	{
		hdr.add("not-a-header;");

		REQUIRE_FALSE(hdr.get("not-a-header"));
		REQUIRE(hdr.empty());
	}
}
