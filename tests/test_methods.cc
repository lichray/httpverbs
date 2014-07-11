#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";
auto k = host + "k";

TEST_CASE("CRUD operations", "[network]")
{
	SECTION("create")
	{
		auto resp = httpverbs::put(k, "ZONE//ALONE");

		REQUIRE(resp.status_code == 201);
	}

	SECTION("read")
	{
		auto resp = httpverbs::get(k);

		REQUIRE(resp.status_code == 200);
		REQUIRE(resp.headers.get("content-type"));
		REQUIRE(resp.content == "ZONE//ALONE");

		resp = httpverbs::head(k);

		REQUIRE(resp.status_code == 200);
		REQUIRE_FALSE(resp.headers.get("content-type"));
		REQUIRE(resp.content.empty());  // needs an evil server
	}

	SECTION("update")
	{
		auto resp = httpverbs::put(k);

		REQUIRE(resp.status_code == 201);

		resp = httpverbs::get(k);

		REQUIRE(resp.status_code == 200);
		REQUIRE(resp.headers.get("content-type"));
		REQUIRE(resp.content.empty());
	}

	SECTION("delete")
	{
		auto resp = httpverbs::delete_(k);

		REQUIRE(resp.status_code == 204);

		resp = httpverbs::head(k);

		REQUIRE(resp.status_code == 404);
	}
}

TEST_CASE("other methods", "[network]")
{
	SECTION("options")
	{
		auto resp = httpverbs::options(host);

		REQUIRE(resp.status_code == 200);
		REQUIRE(resp.headers.get("allow"));
	}

	SECTION("post")
	{
		auto resp = httpverbs::post(host);

		REQUIRE(resp.status_code == 405);
		REQUIRE(resp.headers.get("allow"));
	}
}

TEST_CASE("header overloads", "[network]")
{
	SECTION("url + headers")
	{
		auto hdr = httpverbs::header_dict();
		hdr.add("content-type: text/plain");

		auto resp = httpverbs::put(k, hdr);

		REQUIRE(resp.status_code == 201);
	}

	SECTION("url + headers + data")
	{
		auto hdr = httpverbs::header_dict();
		hdr.add("content-type: text/html");

		auto resp = httpverbs::put(k, hdr, "<p>TERMINATED</p>");

		REQUIRE(resp.status_code == 400);
	}
}
