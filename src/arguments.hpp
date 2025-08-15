#ifndef INCLUDED_ARGUMENTS_HPP
#define INCLUDED_ARGUMENTS_HPP

#include <concepts>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace vb::maker {


template<typename ARGUMENTS>
concept is_argument_list =
    std::ranges::range<ARGUMENTS> && std::same_as<std::ranges::range_value_t<ARGUMENTS>, const char *>;

template<typename ARGUMENTS_DATA>
concept is_argument_container = std::ranges::contiguous_range<ARGUMENTS_DATA> &&
                           std::same_as<std::ranges::range_value_t<ARGUMENTS_DATA>, std::string_view>;

using basic_argument_list = std::span<const char *>;
using argument_container = std::vector<std::string_view>;
using argument_sub_container = std::span<std::string_view>;

static constexpr auto NO_SEPARATOR = '\0';

static_assert(is_argument_list<basic_argument_list>);
static_assert(!is_argument_container<basic_argument_list>);
static_assert(is_argument_container<argument_container>);
static_assert(is_argument_container<argument_sub_container>);

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
constexpr basic_argument_list build_arguments(int argc, const char *argv[])
{
    return std::span{ argv, static_cast<size_t>(argc) };
}

template<is_argument_list ARGUMENTS>
constexpr auto to_arguments(const ARGUMENTS args)
{
    ARGUMENTS all = args;
    return std::ranges::to<std::vector>(all | std::views::transform([](const auto& arg) {
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

constexpr auto
filter_arguments(is_argument_list auto args, char separator, std::same_as<std::string_view> auto... whats)
requires (sizeof...(whats) > 0)
{
    return args | std::views::filter([separator, whats...](auto arg) {
               auto view = std::string_view{ arg };
               if (separator == NO_SEPARATOR) {
                    return ((view == whats) || ...);
               }
               return ((view.starts_with(whats) && (view.size() == whats.size() || view[whats.size()] == separator)) || ...);
           });
}

constexpr auto find_argument(is_argument_list auto args, std::same_as<std::string_view> auto... whats)
    -> std::optional<std::string_view>
requires (sizeof...(whats) > 0)
{
    auto possible = filter_arguments(args, '\0', whats...);
    if (possible.empty()) {
        return std::nullopt;
    }

    return std::string_view{*possible.begin()};
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

}

#endif
