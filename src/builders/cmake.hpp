#ifndef INCLUDED_CMAKE_HPP
#define INCLUDED_CMAKE_HPP

#include "../builder.hpp"
#include "ninja.hpp"
#include "tasks.hpp"
#include "work_directory.hpp"
#include <util/environment.hpp>

#include <filesystem>
#include <string_view>

namespace vb::maker::builders {

struct cmake_spec
{
    static constexpr auto stage      = task_type::configuration;
    static constexpr auto name       = "cmake"sv;
    static constexpr auto build_file = "CMakeLists.txt"sv;
    static constexpr auto command    = "cmake"sv;
    static constexpr auto import_env =
        std::array{ "CMAKE_BUILD_PARALLEL_LEVEL"sv, "CMAKE_BUILD_TYPE"sv, "CMAKE_MODULE_PATH"sv };
};

struct cmake : basic_builder<cmake_spec, cmake>
{
    using basic_builder<cmake_spec, cmake>::create;

    static constexpr auto build_dirs = std::array{
        "build"sv,
    };

    static std::string get_build(env::environment::optional env)
    {
        auto build_dir = env.has_value() ? env->get("BUILD_DIR").value_or(env::variable{ "BUILD_DIR" }).value_str() : ""s;
        build_dir = build_dir.empty()
            ? "build"s
            : build_dir;
        return build_dir;
    }

    cmake(work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
        , my_build_dir{ get_build(env_) }
    {
        environment().set("CMAKE_EXPORT_COMPILE_COMMANDS") = "true";
        environment().set("CMAKE_GENERATOR")               = "Ninja Multi-Config";
    }

    std::string my_build_dir;

private:

    arguments_type get_arguments(std::string_view) const override  {
        return arguments_builder("-B", get_build(environment()));
    }

    bool get_required() const override
    {
        auto build_dir = root().path() / std::filesystem::path{get_build(environment())};
        if (!std::filesystem::is_directory(build_dir)) {
            std::filesystem::create_directory(build_dir);
        }
        return true;
    }

    builder_base::ptr get_next_builder() const override
    {
        auto new_env = environment();
        new_env.import("PATH");
        new_env.set("BUILD_DIR") = my_build_dir;
        return ninja::create(work_dir{ root().path() / my_build_dir }, new_env);
    }
};

}

#endif // INCLUDED_CMAKE_HPP
