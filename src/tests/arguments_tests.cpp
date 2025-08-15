#include <arguments.hpp>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

namespace vb::maker {

// Emulate argv and argc
// NOLINTNEXTLINE[cppcoreguidelines-avoid-c-arrays]
constexpr const char *test_ARGS[] = { "test", "-a", "---config", "-b", "--c-option", "---build", "-d" };

static constexpr auto basic_args = build_arguments(sizeof(test_ARGS) / sizeof(const char *), const_cast<const char **>(test_ARGS));
static inline const auto args =
    to_argument_list(basic_args);

TEST_CASE("Argument_test", "[arguments]")
{
    CHECK(count_arguments(main_arguments(args)) == 2);
    CHECK(*main_arguments(args).begin() == "test"sv);
    auto config_args = to_argument_list(filter_arguments(args, Stage{ task_type::configuration }));
    CHECK_THAT(config_args, Catch::Matchers::RangeEquals({"-b"sv, "--c-option"sv}));
    CHECK(count_arguments(filter_arguments(args, Stage{task_type::configuration})) == 2);
}

}
