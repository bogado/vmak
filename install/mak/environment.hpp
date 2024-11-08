#ifndef INCLUDED_ENVIRONMENT_HPP
#define INCLUDED_ENVIRONMENT_HPP

#include <unistd.h>
#include <util/converters.hpp>

#include <algorithm>
#include <concepts>
#include <iterator>
#include <string_view>
#include <vector>
#include <string>

namespace vb::mak {

struct environment {
    static constexpr auto SEPARATOR = '\0';
    static constexpr auto VALUE_SEPARATOR = '=';

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

    struct mod {
        environment& self;
        std::string update;

        constexpr mod(environment& me, std::string_view n)
          : self{ me }
          , update{ n }
        {}

        void operator=(parse::stringable auto value)
        {
            self.definitions += update + VALUE_SEPARATOR + vb::parse::to_string(value);
            self.definitions.push_back(SEPARATOR);
        }
    };
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

    template <parse::parseable RESULT_T>
    RESULT_T get(std::string name) const {
        auto get_value = [&](auto pos) {
            return parse::from_string<RESULT_T>(std::string_view{pos, std::ranges::find(pos, std::end(definitions), SEPARATOR)});
        };

        auto lookup = name + VALUE_SEPARATOR;
        if (definitions.starts_with(lookup))
        {
            return get_value(std::next(std::begin(definitions), static_cast<int>(lookup.size())));
        }

        lookup.insert(0, std::string{SEPARATOR});
        return get_value(std::ranges::search(definitions, lookup).end());
    };

    auto set(std::string_view name) {
        return mod{*this, name};
    }

    auto size() const
    {
        return std::ranges::count_if(definitions, [](auto c) { return c == SEPARATOR; });
    }
};

}

#endif
