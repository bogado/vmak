#ifndef INCLUDED_PRESETS_HPP
#define INCLUDED_PRESETS_HPP

#include <flat_map>
#include <format>
#include <limits>
#include <utility>

namespace vb::maker {

using namespace std::literals;

enum class task_type : std::size_t
{
    pre_requisites,
    configuration,
    build,
    test,
    install,
    DONE,
    DYNAMIC = std::numeric_limits<std::size_t>::max()
};

static constexpr auto task_type_name(task_type type) {
    static constexpr auto names = std::array {
        "pre-requisites"sv,
        "config"sv,
        "build"sv,
        "test"sv,
        "install"sv
    };

    const auto index = std::to_underlying(type);
    return index <= std::to_underlying(task_type::DONE) 
        ? names.at(index)
        : "undefined"sv;
}

constexpr auto operator<=>(const task_type& type_a, const task_type& type_b)
{
    return std::to_underlying(type_a) <=> std::to_underlying(type_b);
}

constexpr auto next(const task_type& type) {
    if (type == task_type::DONE) {
        return type;
    }
    return task_type{std::to_underlying(type)+1};
}

constexpr auto task_name(task_type type)
{
    static const auto keys = std::flat_map<task_type, std::string_view>{ 
        { task_type::pre_requisites, "pre_requisites"sv},
        { task_type::configuration, "configure"sv},
        { task_type::build, "build"sv},
        { task_type::test, "test"sv },
        { task_type::DONE, "DONE"sv },
        { task_type::DYNAMIC, "dynamic"sv }
    };
    if (auto loc = keys.find(type); loc == std::end(keys)) {
        return loc->second;
    } else {
        return "«UNKNOWN»"sv;
    }
}

}

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
        auto type_key = vb::maker::task_name(type);

        auto out = std::ranges::copy(type_key, context.out()).out;

        return out;
    }
};

#endif // INCLUDED_PRESETS_HPP
