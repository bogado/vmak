#include "builders/cmake_preset.hpp"
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <filesystem>
#include <source_location>
#include <string>

using namespace std::literals;

namespace vb::maker {

std::filesystem::path this_path(std::source_location location= std::source_location::current()) {
    return std::filesystem::absolute(std::filesystem::path{location.file_name()}).parent_path();
}

TEST_CASE("cmake_presets", "[builders,cmake,json]")
{
    auto builder = builders::cmake_preset{work_dir( this_path() / "test_presets/a"), env::environment::optional{}};
    REQUIRE_THAT(std::ranges::to<std::vector>(builder.presets_for(builders::cmake_preset::configuration)), Catch::Matchers::UnorderedEquals(std::vector{{"direct-test"s, "test-included"s}}));
}

}
