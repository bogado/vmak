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
    static constexpr auto stage = task_type::build;
    static constexpr auto name       = "Jekyll"sv;
    static constexpr auto build_file = "_config.yml"sv;
    static constexpr auto command    = "jekyll"sv;
    static constexpr auto arguments  = std::array{ "build"sv };
    static constexpr auto import_env = std::array{ "" };
};

using jekyll = basic_builder<jekyll_spec>;

struct make_spec
{
    static constexpr auto stage = task_type::build;
    static constexpr auto name       = "make"sv;
    static constexpr auto build_file = "Makefile"sv;
    static constexpr auto command    = "make"sv;
};

using make = basic_builder<make_spec>;

struct gnumake_spec
{
    static constexpr auto stage = task_type::build;
    static constexpr auto name       = "gnumake"sv;
    static constexpr auto build_file = "GNUmakefile"sv;
    static constexpr auto command    = "gmake"sv;
};

using gnumake = basic_builder<gnumake_spec>;

struct meson_spec
{
    static constexpr auto stage = task_type::build;
    static constexpr auto name       = "Meson"sv;
    static constexpr auto build_file = "meson.build"sv;
    static constexpr auto command    = "meson"sv;
    static constexpr auto next_step  = ninja::create;
};

using meson =  basic_builder<meson_spec>;

struct cargo_spec
{
    static constexpr auto stage = task_type::build;
    static constexpr auto name       = "Cargo"sv;
    static constexpr auto build_file = "cargo.toml"sv;
    static constexpr auto command    = "cargo"sv;
    static constexpr auto next_step  = ninja::create;
    static constexpr auto arguments  = std::array{ "build"sv };
};

using cargo = basic_builder<cargo_spec>;

inline constexpr auto all_factories =
    std::array{ make::create, gnumake::create, cmake_preset::create, cmake::create, ninja::create, jekyll::create, cargo::create, meson::create};

builder_base::ptr select(work_dir root, env::environment::optional env = {})
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
