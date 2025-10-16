#ifndef INCLUDED_BUILDERS_HPP
#define INCLUDED_BUILDERS_HPP

#include "builder.hpp"
#include "builders/cmake.hpp"
#include "builders/cmake_preset.hpp"
#include "builders/conan.hpp"
#include "builders/meson.hpp"
#include "builders/ninja.hpp"
#include "tasks.hpp"
#include <util/environment.hpp>

#include <filesystem>
#include <optional>
#include <string_view>

namespace vb::maker::builders {

using namespace vb::literals;
using namespace std::literals;

bool is_git_root(std::filesystem::path path)
{
    auto git = path / ".git";
    return std::filesystem::is_directory(git) || std::filesystem::is_regular_file(git);
}

optional_path git_root_locator(std::filesystem::path path)
{
    while (!is_git_root(path) && (path.has_relative_path())) {
        path = path.parent_path();
    }
    return (path.has_relative_path()) ? optional_path{ path } : std::nullopt;
}

struct jekyll_spec
{
    static constexpr auto stage      = task_type::build;
    static constexpr auto name       = "Jekyll"sv;
    static constexpr auto build_file = "_config.yml"sv;
    static constexpr auto command    = "bundle"sv;
    static constexpr auto arguments  = std::array{ "exec"sv, "jekyll"sv, "build"sv };
    static constexpr auto import_env = std::array{ "" };
};

using jekyll = basic_builder<jekyll_spec>;

struct make_spec
{
    static constexpr auto stage      = task_type::build;
    static constexpr auto name       = "make"sv;
    static constexpr auto build_file = "Makefile"sv;
    static constexpr auto command    = "make"sv;
};

using make = basic_builder<make_spec>;

struct gnumake_spec
{
    static constexpr auto stage      = task_type::build;
    static constexpr auto name       = "gnumake"sv;
    static constexpr auto build_file = "GNUMakefile"sv;
    static constexpr auto command    = "gmake"sv;
};

using gnumake = basic_builder<gnumake_spec>;

struct cargo_spec
{
    static constexpr auto stage      = task_type::build;
    static constexpr auto name       = "Cargo"sv;
    static constexpr auto build_file = "cargo.toml"sv;
    static constexpr auto command    = "cargo"sv;
    static constexpr auto next_step  = ninja::create;
    static constexpr auto arguments  = std::array{ "build"sv };
};

using cargo = basic_builder<cargo_spec>;

struct gradle_spec
{
    static constexpr auto stage      = task_type::build;
    static constexpr auto name       = "gradle"sv;
    static constexpr auto build_file = std::array{ "gradlew"sv };
    static constexpr auto command    = "./gradlew"sv;
    static constexpr auto arguments  = std::array{ "assemble"sv };
};

using gradle = basic_builder<gradle_spec>;

struct factory
{
    builder_base::factory my_builder;
    task_type             my_type;
    std::string_view      my_name;

    bool (*my_stage_check)(task_type);

    template<is_builder BUILDER_T>
    static constexpr auto accepts_type(task_type stage)
    {
        using SPEC_T = BUILDER_T::specification_type;
        if constexpr (SPEC_T::stage == task_type::DYNAMIC) {
            return std::ranges::find(SPEC_T::stages, stage) != std::end(SPEC_T::stages);
        } else {
            return SPEC_T::stage == stage;
        }
    }

    template<is_builder BUILDER_T>
    static constexpr factory for_class()
    {
        return factory{ BUILDER_T::create, BUILDER_T::stage, BUILDER_T::builder_name, &accepts_type<BUILDER_T> };
    }
};

inline constexpr auto all_factories =
    std::array{ factory::for_class<make>(),  factory::for_class<gnumake>(), factory::for_class<cmake_preset>(),
                factory::for_class<cmake>(), factory::for_class<ninja>(),   factory::for_class<jekyll>(),
                factory::for_class<cargo>(), factory::for_class<meson>(),   factory::for_class<gradle>(),
                factory::for_class<conan>() };

builder_base::ptr select(work_dir root, Stage stage, env::environment::optional env = {})
{
    for (const auto& factory : all_factories | std::views::filter([&](const auto& factory) {
        return factory.my_stage_check(stage.type());
    })) {
        if (auto ptr = factory.my_builder(root, env); ptr != nullptr) {
            return ptr;
        }
    }
    return builder_base::ptr{ nullptr };
}

}

#endif // INCLUDED_BUILDERS_HPP
