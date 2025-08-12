#ifndef INCLUDED_CMAKE_HPP
#define INCLUDED_CMAKE_HPP

#include "../builder.hpp"
#include "ninja.hpp"
#include "tasks.hpp"
#include <util/environment.hpp>

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
        , build_dir{ get_build(env_) }
    {
        environment().set("CMAKE_EXPORT_COMPILE_COMMANDS") = "true";
        environment().set("CMAKE_GENERATOR")               = "Ninja Multi-Config";
    }

    std::string build_dir;

private:

    arguments_type get_arguments(std::string_view target) const override {
        auto arguments = arguments_builder("--build"sv, build_dir);

         if (!target.empty()) {
             auto target_args = std::array{ "--target"sv, target};
             arguments.insert_range(std::end(arguments), target_args | std::views::transform([](auto v) { return std::string{v}; }));
         }
         return arguments;
    }

    bool get_required() const override
    {
        return std::ranges::any_of(build_dirs, [&](const auto& folder_name) {
            return root().has_folder(folder_name);
        });
    }

    builder_base::ptr get_next_builder() const override
    {
        auto new_env = environment();
        new_env.import("PATH");
        new_env.set("BUILD_DIR") = build_dir;
        return ninja::create(work_dir{ root().path() / build_dir }, new_env);
    }
};

}

#endif // INCLUDED_CMAKE_HPP
