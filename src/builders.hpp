#ifndef INCLUDED_BUILDERS_HPP
#define INCLUDED_BUILDERS_HPP

#include "builder.hpp"
#include "builders/cmake.hpp"
#include "builders/cmake_preset.hpp"
#include "builders/conan.hpp"
#include "builders/ninja.hpp"
#include <util/environment.hpp>

#include <string_view>

namespace vb::maker::builders {

using namespace vb::literals;
using namespace std::literals;

struct jekyll_spec
{
    static constexpr std::string_view name       = "Jekyll";
    static constexpr std::string_view build_file = "_config.yml";
    static constexpr std::string_view command    = "jekyll";
    static constexpr std::array       arguments  = { "build"sv };
};

using jekyll = basic_builder<jekyll_spec>;

struct make_spec
{
    static constexpr std::string_view name       = "make";
    static constexpr std::string_view build_file = "Makefile";
    static constexpr std::string_view command    = "make";
};

using make = basic_builder<make_spec>;

inline const auto all_factories =
    std::array{ make::create, conan::create, cmake_preset::create, cmake::create, ninja::create, jekyll::create };

builder_base::ptr
select(work_dir root, env::environment::optional env = {})
{
    for (const auto& factory : all_factories) {
        if (auto ptr = factory(root, env); ptr != nullptr) {
            return ptr;
        }
    }
    return builder_base::ptr{ nullptr };
}

}

#endif // INCLUDED_BUILDERS_HPP
