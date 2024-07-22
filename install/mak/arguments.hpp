// arguments.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_ARGUMENTS_HPP
#define INCLUDED_ARGUMENTS_HPP

#include <string_view>
#include <vector>
#include <ranges>
#include <span>

#include <util/string.hpp>

namespace vb::mak {

class arguments {
    std::vector<const char*> args;
public:

    template <std::ranges::view VIEWABLE>
    arguments(VIEWABLE view) :
        args{std::begin(view), std::end(view)}
    {}

    arguments(const char** argc, std::size_t argv) :
        arguments{std::span(argc, argv)}
    {}

    template <unsigned ... SIZEs>
    constexpr arguments(static_string<SIZEs>... arguments_) : 
        args{arguments_.array()...}
    {}

    constexpr std::string_view operator[](std::size_t n) {
        if (n >= std::size(args)) {
            return {};
        }
        return args[n];
    }
};

}

#endif
