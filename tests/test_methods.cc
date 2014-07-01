#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <httpverbs/httpverbs.h>

httpverbs::enable_library _;
std::string host = "http://localhost:8080/";

TEST_CASE("CRUD operations", "[network]")
{
	auto k = host + "k";

	SECTION("create")
	{
		auto resp = httpverbs::put(k, "ZONE//ALONE");

		REQUIRE(resp.status_code == 201);
	}

	SECTION("read")
	{
		auto resp = httpverbs::get(k);

		REQUIRE(resp.status_code == 200);
		REQUIRE(resp.get_header("content-type"));
		REQUIRE(resp.content == "ZONE//ALONE");

		resp = httpverbs::head(k);

		REQUIRE(resp.status_code == 200);
		REQUIRE_FALSE(resp.get_header("content-type"));
		REQUIRE(resp.content.empty());  // needs an evil server
	}

	SECTION("update")
	{
		auto resp = httpverbs::put(k);

		REQUIRE(resp.status_code == 201);

		resp = httpverbs::get(k);

		REQUIRE(resp.status_code == 200);
		REQUIRE(resp.get_header("content-type"));
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
		REQUIRE(resp.get_header("allow"));
	}

	SECTION("post")
	{
		auto resp = httpverbs::post(host);

		REQUIRE(resp.status_code == 405);
		REQUIRE(resp.get_header("allow"));
	}
}
