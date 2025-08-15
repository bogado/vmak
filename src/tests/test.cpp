#include "builders.hpp"
#include "builders/cmake_preset.hpp"
#include "builders/conan.hpp"
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <exception>
#include <filesystem>
#include <format>
#include <ranges>
#include <source_location>
#include <string>

using namespace std::literals;

template<typename T>
struct Catch::StringMaker<std::optional<T>> {
    static std::string convert(const std::optional<T>& optional ) {
        if (optional.has_value()) {
            return Catch::StringMaker<T>::convert(optional.value());
        } else {
            return "«EMPTY»"s;
        }
    }
};

namespace vb::maker {

inline const std::filesystem::path this_path = [](std::source_location location) {
    return std::filesystem::absolute(std::filesystem::path{ location.file_name() }).parent_path();
}(std::source_location::current());

inline const std::filesystem::path conan_test_configuration = this_path / "conan";
inline const std::filesystem::path cmake_test_presets       = this_path / "test_presets/a";

TEST_CASE("cmake_presets", "[builders][cmake][json]")
{
    SKIP();
    auto builder = builders::cmake_preset{ work_dir{ cmake_test_presets }, env::environment::optional{} };
    REQUIRE_THAT(
        std::ranges::to<std::vector>(builder.presets_for(builders::cmake_preset::configuration)),
        Catch::Matchers::UnorderedEquals(std::vector{ { "direct-test"s, "test-included"s } }));
}

TEST_CASE("conan_config", "[conan][config]")
{
    SKIP();
    try {
        auto config = builders::details::conan_configuration{ conan_test_configuration };
        REQUIRE_THAT(
            config.keys(),
            Catch::Matchers::UnorderedRangeEquals(
                std::array{ "/tools.cmake.cmaketoolchain:generator"s,
                            "/tools.cmake.cmake_layout:build_folder_vars/0"s }));
    } catch (const std::exception& error) {
        FAIL(std::format("failure to load configuration: {}", error.what()));
    }
}

TEST_CASE("conan_profile_config", "[conan][config][profile]")
{
    static const auto data = std::array{
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/settings/compiler.version"sv, "19"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/settings/compiler.libcxx"sv, "libstdc++11"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/settings/compiler.cppstd"sv, "23"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/settings/compiler"sv, "clang"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/settings/build_type"sv, "Debug"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/settings/arch"sv, "x86_64"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/package_settings"sv, std::nullopt },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/options"sv, std::nullopt },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/conf/tools.cmake.cmake_layout:build_folder_vars/0"sv,
                                                            "settings.compiler"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/build_env"sv, "CXX=clang++\nCC=clang\n"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/conf/tools.cmake.cmaketoolchain:generator"sv,
                                                            "Ninja Multi-Config"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/settings/compiler.libcxx"sv, "libstdc++11"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/options"sv, std::nullopt },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/tool_requires"sv, std::nullopt },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/settings/arch"sv, "x86_64"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/package_settings"sv, std::nullopt },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/settings/build_type"sv, "Debug"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/settings/os"sv, "Linux"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/build_env"sv, "CXX=clang++\nCC=clang\n"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/settings/compiler"sv, "clang"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/settings/compiler.cppstd"sv, "23"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/settings/os"sv, "Linux"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/conf/tools.cmake.cmake_layout:build_folder_vars/0"sv,
                                                            "settings.compiler"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/settings/compiler.version"sv, "19"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/host/conf/tools.cmake.cmaketoolchain:generator"sv,
                                                            "Ninja Multi-Config"sv },
        std::pair<std::string_view, std::optional<std::string_view>>{ "/build/tool_requires"sv, std::nullopt }
    };
    try {
        auto config = builders::details::conan_configuration::from_profile(conan_test_configuration, "clang");
        CHECK_THAT(config.keys(), Catch::Matchers::UnorderedRangeEquals(data | std::views::keys));
        CHECK_THAT(config.values(), Catch::Matchers::UnorderedRangeEquals(data | std::views::values));
    } catch (const std::exception& error) {
        FAIL(std::format("failure to load configuration: {}", error.what()));
    }
}

}
