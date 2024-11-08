#include <catch2/catch_all.hpp>

#include "environment.hpp"
#include "bounded_array.hpp"

using namespace vb::mak; 

TEST_CASE("environment_test", "[env]")
{
    environment env_test;
    env_test.set("test") = "value";
    REQUIRE(env_test.size() == 1);

    env_test.set("check") = 32;
    REQUIRE(env_test.size() == 2);

    REQUIRE(env_test.get<std::string>("test") == "value");
    REQUIRE(env_test.get<std::string>("check") == "32");
    REQUIRE(env_test.get<int>("check") == 32);
}

