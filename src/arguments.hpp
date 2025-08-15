#ifndef INCLUDED_ARGUMENTS_HPP
#define INCLUDED_ARGUMENTS_HPP

#include "tasks.hpp"
#include <util/option/concepts.hpp>

#include <concepts>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace vb::maker {

constexpr auto STAGE_SEPARATOR = "---"sv;

template<typename ARGUMENTS>
concept is_argument_list =
    std::ranges::range<ARGUMENTS> && std::constructible_from<std::string_view, std::ranges::range_value_t<ARGUMENTS>>;

template<typename ARGUMENTS>
concept is_argument_view = 
    is_argument_list<ARGUMENTS> && std::ranges::view<ARGUMENTS>;

using basic_argument_list = std::span<const char *>;
using standard_argument_list = std::vector<std::string_view>;

static_assert(is_argument_list<standard_argument_list>);
static_assert(is_argument_list<basic_argument_list>);
static_assert(is_argument_view<basic_argument_list>);

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
constexpr basic_argument_list build_arguments(int argc, const char *argv[])
{ 
    return std::span{ argv, static_cast<size_t>(argc) };
}

constexpr auto to_argument_list(is_argument_view auto args)
{
    return std::ranges::to<std::vector>(args | std::views::transform([](const auto& arg) {
               return std::string_view{ arg };
           }));
}

constexpr auto command_from_args(is_argument_list auto args)
{
    if (args.empty()) {
        return std::string_view{};
    }
    return std::string_view{ *args.begin() };
}

constexpr auto get_arguments(is_argument_list auto args, std::string_view what)
{
    return args | std::views::filter([argument = what](const auto& arg) {
               return arg.starts_with(argument);
           }) |
           std::views::transform([size = what.size()](const auto& arg) {
               return std::string_view{ arg }.substr(0, size);
           });
}

constexpr auto find_argument(is_argument_list auto args, char separator, std::same_as<std::string_view> auto... whats)
    -> std::optional<std::string_view>
{
    auto list = to_argument_list(args);
    for (auto what : { whats... }) {
        auto it = std::ranges::find_if(list, [&](const auto& arg) {
            return arg.starts_with(what) && (arg.size() == what.size() || arg[what.size()] == separator);
        });
        if (it != std::end(list)) {
            return { what };
        }
    }

    return std::nullopt;
}

constexpr auto find_argument(is_argument_list auto args, std::same_as<std::string_view> auto... whats)
    -> std::optional<std::string_view>
{
    return find_argument(args, '\0', whats...);
}

constexpr std::size_t count_arguments(is_argument_list auto args)
{
    return static_cast<std::size_t>(std::ranges::distance(std::begin(args), std::end(args)));
}

constexpr std::size_t count_arguments(is_argument_list auto args, std::string_view arg)
{
    return count_arguments(get_arguments(arguments_as_view(args), arg));
}

constexpr auto drop_command(is_argument_list auto args)
{
    return args | std::views::drop(1);
}

constexpr auto main_arguments(is_argument_list auto args)
{
    return args | std::views::take_while([](const auto& arg) {
               return !std::string_view{ arg }.starts_with(STAGE_SEPARATOR);
           });
}

static_assert(is_argument_list<std::remove_cvref_t<decltype(main_arguments(std::span<const char *>{}))>>);
constexpr auto filter_arguments(is_argument_list auto args, Stage stage)
{
    auto boundary_check = [&](bool bound) {
        return [&](const auto& arg) {
            auto view = std::string_view{ arg };
            if (!view.starts_with(STAGE_SEPARATOR)) {
                return true;
            }

            auto rest = view.substr(STAGE_SEPARATOR.size()) != stage.option();
            return bound ? rest : !rest;
        };
    };

    return args | std::views::drop_while(boundary_check(true)) | std::views::drop(1) |
           std::views::take_while(boundary_check(false));
};

}

#endif
