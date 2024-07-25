// arguments.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_ARGUMENTS_HPP
#define INCLUDED_ARGUMENTS_HPP

#include <string_view>
#include <vector>
#include <span>

#include <util/string.hpp>

namespace vb::mak {

class arguments {
    std::vector <std::string> args;

public:
    auto size() const {
        return args.size();
    }

    template <std::ranges::view VIEWABLE>
    arguments(VIEWABLE view) :
        args{std::begin(view), std::end(view)}
    {}

    arguments(const char** argc, std::size_t argv) :
        arguments{std::span(argc, argv)}
    {}

    template <size_t... SIZEs>
    constexpr arguments(const char (&...args)[SIZEs]) : 
        args{args...}
    {}

    constexpr std::string_view operator[](std::size_t n) const {
        return args[n];
    }
};

}

#endif
