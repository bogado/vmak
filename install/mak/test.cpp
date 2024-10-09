#include <catch2/cetch_all.hpp>

#include "environment.hpp"

using namespace vb; 

TEST_CASE("environment_test", "[env]")
{
    environment env_test;
    env_test.set("test").to("value");
    REQUIRE(test.size() == 1);
}

