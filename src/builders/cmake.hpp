#ifndef INCLUDED_CMAKE_HPP
#define INCLUDED_CMAKE_HPP

#include "../builder.hpp"
#include "ninja.hpp"
#include <util/environment.hpp>

#include <string_view>

namespace vb::maker::builders {

struct cmake_spec
{
    static constexpr std::string_view name       = "cmake";
    static constexpr std::string_view build_file = "CMakeLists.txt";
    static constexpr std::string_view command    = "cmake";
    static constexpr auto import_env = std::array {
        "CMAKE_BUILD_PARALLEL_LEVEL"sv,
        "CMAKE_BUILD_TYPE"sv,
        "CMAKE_MODULE_PATH"sv
    };
};

struct cmake : basic_builder<cmake_spec, cmake>
{
    using basic_builder<cmake_spec, cmake>::create;

    static constexpr auto build_dirs = std::array{
        "build"sv,
    };

    static std::string get_build(env::environment::optional env)
    {
        return env.has_value() ? env->get("BUILD_DIR").value_or(env::variable{ "BUILD_DIR" }).value_str() : "build"s;
    }

    cmake(work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
        , build_dir{ get_build(env_) }
    {
        environment().set("CMAKE_EXPORT_COMPILE_COMMANDS") = "true";
        environment().set("CMAKE_GENERATOR")               = "Ninja Multi-Config";
    }

    std::string build_dir;

    execution_result execute(std::string_view) const override // target ignored in this step.
    {
        using namespace std::literals;
        if (environment().contains("CMAKE_PRESET")) {
            auto profile = environment().value_for("CMAKE_PRESET").value_or("default");
            println("\tprofile: {}", profile);
            return root().execute(command, std::array{ "--profile"s, profile }, environment());
        } else {
            return root().execute(command, std::array{ "-B"s, build_dir }, environment());
        }
    }

    bool required() const override
    {
        return std::ranges::any_of(build_dirs, [&](const auto& folder_name) { return root().has_folder(folder_name); });
    }

    builder_base::ptr next_builder() const override
    {
        auto new_env = environment();
        new_env.import("PATH");
        new_env.set("BUILD_DIR") = build_dir;
        return ninja::create(work_dir{ root().path() / build_dir }, new_env);
    }
};

}

#endif // INCLUDED_CMAKE_HPP
