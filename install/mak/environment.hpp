#ifndef INCLUDED_ENVIRONMENT_HPP
#define INCLUDED_ENVIRONMENT_HPP

#include <util/converters.hpp>
#include <util/string.hpp>

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
        environment& self; // NOLINT: cppcoreguidelines-avoid-const-or-ref-data-members
        std::string update;

        mod(environment& me, std::string_view n)
          : self{ me }
          , update{ n }
        {}

        void operator=(parse::stringable auto value) // NOLINT: cppcoreguidelines-c-copy-assignment-signature
        {
            self.definitions += update + VALUE_SEPARATOR + vb::parse::to_string(value);
            self.definitions.push_back(SEPARATOR);
        }
    };

    auto lookup_name(std::string lookup) const
    {
        lookup += VALUE_SEPARATOR;
        if (definitions.starts_with(lookup))
        {
            return std::next(std::begin(definitions), static_cast<int>(lookup.size() - 1));
        }

        lookup.insert(0, std::string{SEPARATOR});
        auto found = std::ranges::search(definitions, lookup);
        if (found.begin() == definitions.end()) {
            return std::end(definitions);
        }
        return std::prev(found.end());
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

    template <parse::parseable RESULT_T>
    RESULT_T get(std::string name) const {
        auto pos = lookup_name(name);
        return from_string<RESULT_T>(std::string_view(pos, std::find(pos, definitions.end(), SEPARATOR)));
    }

    bool contains(std::string name) {
        return lookup_name(std::string{name}) != definitions.end();
    }
        
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
