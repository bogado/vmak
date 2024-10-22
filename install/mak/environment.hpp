// environment.hpp                                                                        -*-C++-*-

#ifndef INCLUDED_ENVIRONMENT_HPP
#define INCLUDED_ENVIRONMENT_HPP

#include <unistd.h>

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>
#include <vector>
#include <string>

namespace vb::mak {

struct environment {
    static constexpr auto SEPARATOR = '\0';

private:
    std::string definitions;
    std::vector<char *> environ;

    void updateCache()
    {
        environ.clear();
        bool ended = true;
        for(auto& chr: definitions) {
            if (ended && chr != SEPARATOR) {
                environ.push_back(&chr);
                ended = false;
            }
            if (chr == SEPARATOR) {
                ended = true;
            }
        }
    }
public:
    environment() = default;

    explicit environment(const char **environment_)
    {
        while(environment_ != nullptr) {
            definitions += std::string(*environment_);
            definitions.push_back(SEPARATOR);
            environment_ = std::next(environment_);
        }
    }

    auto set(std::string_view name) {
        auto setter_ = [&](std::string_view value)
        {
            definitions += std::string(name) + "=" + std::string(value);
            definitions.push_back(SEPARATOR);
        };

        struct setter {
            decltype(setter_) to = setter_;
        };

        return setter{};
    }

    auto size() const
    {
        return std::ranges::count_if(definitions, [](auto c) { return c == SEPARATOR; });
    }
};

} // namespace BloombergLP

#endif
