// arguments.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_ARGUMENTS_HPP
#define INCLUDED_ARGUMENTS_HPP

#include <array>
#include <complex>
#include <string_view>

#include <util/string.hpp>

namespace vb {

template <std::size_t... SIZEs>
class arguments {
    std::tuple<std::array<char, SIZEs>...> raw_arguments;

    template <std::size_t N>
    constexpr std::string_view getter(std::size_t n = N) const {
        if constexpr (N >= count) {
            return std::string_view{};
        } else {

            if (n > N) { 
                return std::string_view{};
            }

            if (n < N) {
                return getter<N+1>(n);
            }

            const auto& arg = std::get<N>(raw_arguments);
            return std::string_view{arg.data(), arg.size()};
        }
    }

public:
    static constexpr auto count = sizeof...(SIZEs);
    
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
