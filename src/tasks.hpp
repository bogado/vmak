#ifndef INCLUDED_PRESETS_HPP
#define INCLUDED_PRESETS_HPP

#include <compare>
#include <flat_map>
#include <format>
#include <limits>
#include <ranges>
#include <utility>

namespace vb::maker {

using namespace std::literals;

enum class task_type : std::size_t
{
    pre_requisites,
    configuration,
    build,
    test,
    package,
    install,
    DONE,
    COUNT   = DONE,
    DYNAMIC = std::numeric_limits<std::size_t>::max()
};

constexpr std::strong_ordering operator<=>(const task_type& type_a, const task_type& type_b)
{
    return std::to_underlying(type_a) <=> std::to_underlying(type_b);
}

struct Stage
{
    constexpr std::strong_ordering operator<=>(const Stage&) const = default;

    struct Information
    {
        std::string_view name;
        std::string_view option;
        std::string_view information;
    };

    constexpr Stage() = default;

    constexpr explicit Stage(task_type type)
        : my_task_type{ type }
    {
    }

    constexpr auto name() const
    {
        return get_info(my_task_type).name;
    }

    constexpr auto option() const
    {
        return get_info(my_task_type).option;
    }

    constexpr auto information() const
    {
        return get_info(my_task_type).information;
    }

    constexpr auto next() const
    {
        if (my_task_type == task_type::DONE) {
            return *this;
        }
        return Stage{ task_type{ std::to_underlying(my_task_type) + 1 } };
    }

    constexpr auto type() const
    {
        return my_task_type;
    }

private:

    task_type my_task_type = task_type::DONE;

    static constexpr auto get_info(task_type type) -> Information
    {
        static constexpr auto infos = std::array{
            std::pair{ task_type::pre_requisites,
                       Information{ "pre_requisites"sv, "prereq", "Install pre-requisites using a package manager" } },
            std::pair{ task_type::configuration,
                       Information{ "configure"sv, "conf"sv, "Build configuration stage."sv } },
            std::pair{ task_type::build, Information{ "build"sv, "build"sv, "Build stage."sv } },
            std::pair{ task_type::test, Information{ "test"sv, "test"sv, "Test stage."sv } },
            std::pair{ task_type::package, Information{ "package"sv, "pack"sv, "Packaging stage."sv } },
            std::pair{ task_type::DONE, Information{ "DONE"sv, ""sv, ""sv } },
            std::pair{ task_type::DYNAMIC, Information{ "dynamic"sv, ""sv, ""sv } }
        };

        if (auto it = std::ranges::find(infos | std::views::keys, type); it.base() != std::end(infos)) {
            return it.base()->second;
        } else {
            return Information{ "NO TASK"sv, ""sv, "Invalid stage"sv };
        }
    }
};

static constexpr auto all_stages = []() {
    std::array<Stage, std::to_underlying(task_type::DONE)> result{};
    std::ranges::copy(
        std::ranges::iota_view(std::size_t{ 0 }) | std::views::transform([](auto x) {
            return Stage{ task_type{ x } };
        }) | std::views::take_while([](auto stage) {
            return stage != Stage{ task_type::DONE };
        }),
        result.begin());
    return result;
}();

}

template<>
struct std::formatter<vb::maker::Stage, char>
{
    template<class PARSE_CONTEXT>
    constexpr PARSE_CONTEXT::iterator parse(PARSE_CONTEXT& context)
    {
        return context.begin();
    }

    template<class FORMAT_CONTEXT>
    constexpr FORMAT_CONTEXT::iterator format(const vb::maker::Stage& type, FORMAT_CONTEXT& context) const
    {
        auto type_key = type.name();

        auto out = std::ranges::copy(type_key, context.out()).out;

        return out;
    }
};

template<>
struct std::formatter<vb::maker::task_type, char>
{
    template<class PARSE_CONTEXT>
    constexpr PARSE_CONTEXT::iterator parse(PARSE_CONTEXT& context)
    {
        return context.begin();
    }

    template<class FORMAT_CONTEXT>
    constexpr FORMAT_CONTEXT::iterator format(const vb::maker::task_type& type, FORMAT_CONTEXT& context) const
    {
        auto type_key = vb::maker::Stage{ type }.name();

        auto out = std::ranges::copy(type_key, context.out()).out;

        return out;
    }
};

#endif // INCLUDED_PRESETS_HPP
