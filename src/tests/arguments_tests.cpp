#include "tasks.hpp"
#include "arguments.hpp"

#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <vector>
#include <string_view>

namespace vb::maker {

// Emulate argv and argc
// NOLINTNEXTLINE[cppcoreguidelines-avoid-c-arrays]
constexpr const char *test_ARGS[] = { "test", "-a", "---config", "-b", "--c-option", "---build", "-d" };

static constexpr auto args = build_arguments(sizeof(test_ARGS) / sizeof(const char *), const_cast<const char **>(test_ARGS));
static inline const auto arg_list = to_arguments(args);

TEST_CASE("Argument_test", "[arguments][stages]")
{
    CHECK_THAT(to_arguments(Stage::main_arguments(args)), Catch::Matchers::RangeEquals({"test"sv, "-a"sv}));
    CHECK_THAT(Stage::main_arguments(arg_list), Catch::Matchers::RangeEquals({"test"sv, "-a"sv}));

    CHECK_THAT(to_arguments(Stage{ task_type::configuration }.filter_arguments(args)), Catch::Matchers::RangeEquals({"-b"sv, "--c-option"sv}));
    CHECK_THAT(Stage{ task_type::configuration }.filter_arguments(arg_list), Catch::Matchers::RangeEquals({"-b"sv, "--c-option"sv}));
}


TEST_CASE("Empty_argument_list", "[arguments][not_present]")
{
    CHECK_THAT(to_arguments(Stage{task_type::test}.filter_arguments(args)), Catch::Matchers::RangeEquals(std::vector<std::string_view>{}));
    CHECK_THAT(Stage{task_type::test}.filter_arguments(arg_list), Catch::Matchers::RangeEquals(std::vector<std::string_view>{}));
}

}
