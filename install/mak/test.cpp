#include <catch2/catch_all.hpp>

#include "environment.hpp"

using namespace vb::mak; 

TEST_CASE("environment_test", "[env]")
{
    environment env_test;
    env_test.set("test").to("value");
    REQUIRE(env_test.size() == 1);
}

