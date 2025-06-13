#ifndef INCLUDED_PRESETS_HPP
#define INCLUDED_PRESETS_HPP

#include <array>
#include <format>

namespace vb::maker {

using namespace std::literals;

enum class preset_type : char
{
    configuration,
    build,
    test
};

constexpr auto preset_type_name(preset_type type)
{
    static auto keys = std::array{ "configure"sv, "build"sv, "test"sv };
    return keys[static_cast<std::size_t>(type)];
}

}

template<>
struct std::formatter<vb::maker::preset_type, char>
{
    template<class PARSE_CONTEXT>
    constexpr PARSE_CONTEXT::iterator parse(PARSE_CONTEXT& context)
    {
        return context.begin();
    }

    template<class FORMAT_CONTEXT>
    constexpr FORMAT_CONTEXT::iterator format(const vb::maker::preset_type& type, FORMAT_CONTEXT& context) const
    {
        auto type_key = vb::maker::preset_type_name(type);

        auto out = std::ranges::copy(type_key, context.out()).out;

        return out;
    }
};

#endif // INCLUDED_PRESETS_HPP
