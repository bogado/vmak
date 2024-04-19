// arguments.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_ARGUMENTS_HPP
#define INCLUDED_ARGUMENTS_HPP

#include <complex>
#include <string_view>
#include <vector>
#include <ranges>

#include <util/string.hpp>

namespace vb {

class arguments {
    std::vector <std::string_view> args;

public:

    template <std::ranges::view VIEWABLE>
    arguments(VIEWABLE view) :
        args{std::begin(view), std::end(view)}
    {}

    arguments(const char** argc, std::size_t argv) :
        arguments{std::span(argc, argv)}
    {}

    constexpr arguments(static_string<SIZEs>... args) : 
        raw_arguments{args.array()...}
    {}

    constexpr std::string_view operator[](std::size_t n) {
        return getter<0>(n);
    }
};

template <std::size_t ... SIZEs>
arguments(const char(&...arrs)[SIZEs]) -> arguments<SIZEs...>;

static_assert(arguments("test", "12", "3").count == 3);

}

#endif
