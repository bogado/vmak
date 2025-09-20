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
    static constexpr auto build_dir = true;
    static constexpr auto creates   = "build.ninja"sv;
};

struct cmake : basic_builder<cmake_spec, cmake>
{
    using basic_builder<cmake_spec, cmake>::create;

    cmake(work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
    {
        environment().set("CMAKE_EXPORT_COMPILE_COMMANDS") = "true";
        environment().set("CMAKE_GENERATOR")               = "Ninja Multi-Config";
    }

private:

    arguments_type get_arguments(std::string_view) const override  {
        return arguments_builder("-B", get_build_dirname(environment()));
    }

    builder_base::ptr get_next_builder() const override
    {
        return builder_base::ptr{std::make_unique<ninja>(root(), environment(), get_build_dir())};
    }
};

}

#endif // INCLUDED_CMAKE_HPP
