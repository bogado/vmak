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

template <typename T>
struct nullptr_bounded_array {
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = nullptr_bounded_array;

    pointer *head;

    auto begin() {
        return *this;
    }

    auto end() {
        return std::default_sentinel;
    }

    bool operator==(std::default_sentinel_t)
    {
        return *head == nullptr;
    }

    friend bool operator==(std::default_sentinel_t, const nullptr_bounded_array& self)
    {
        return self != std::default_sentinel;
    }

    auto operator++()
    {
        head++;
        return *this;
    }

    auto operator++(int)
    {
        auto result = *this;
        head++;
        return result;
    }

    bool operator==(nullptr_bounded_array&) const = default;
};

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
