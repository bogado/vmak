// arguments.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_ARGUMENTS_HPP
#define INCLUDED_ARGUMENTS_HPP

#include <string_view>
#include <vector>
#include <span>

#include <util/string.hpp>

namespace vb::mak {

class arguments {
    std::vector <std::string> argument_values;
public:
    auto size() const {
        return argument_values.size();
    }

    template <std::ranges::view VIEWABLE>
    arguments(VIEWABLE view) :
        argument_values{std::begin(view), std::end(view)}
    {}

    arguments(const char** argc, std::size_t argv) :
        arguments{std::span(argc, argv)}
    {}

    template <unsigned ... SIZEs>
    constexpr arguments(static_string<SIZEs>... arguments_) : 
        argument_values{arguments_.array()...}
    {}

    template <size_t... SIZEs>
    constexpr arguments(const char (&...args)[SIZEs]) : 
        argument_values{args...}
    {}

    constexpr std::string_view operator[](std::size_t n) const {
        if (n >= std::size(argument_values)) {
            return {};
        }
        return {argument_values[n]};
    }
};

}

#endif
