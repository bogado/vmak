#ifndef INCLUDED_CONAN_HPP
#define INCLUDED_CONAN_HPP

#include "../builder.hpp"
#include "cmake.hpp"
#include "util/environment.hpp"
#include <unistd.h>

#include <filesystem>
#include <map>
#include <print>
#include <ranges>

namespace vb::maker::builders {

using namespace env::literals;

struct conan_spec
{
    static constexpr std::string_view name       = "conan";
    static constexpr std::string_view build_file = "conanfile.txt";
    static constexpr std::string_view command    = "conan";
};

struct conan_profile
{
    constexpr conan_profile(std::string_view repr)
        : name{ repr.substr(0, repr.find(':')) }
        , build_dir{ repr.substr(name.size() + 1, repr.find('/') - name.size() - 1) }
        , generator_dir{ repr.substr(name.size() + build_dir.size() + 2) }
    {
    }

    std::string_view name;
    std::string_view build_dir;
    std::string_view generator_dir;
};

static_assert(conan_profile{ "test:build_test/test/generator"sv }.name == "test");
static_assert(conan_profile{ "test:build_test/test/generator"sv }.build_dir == "build_test");
static_assert(conan_profile{ "test:build_test/test/generator"sv }.generator_dir == "test/generator");

struct conan : basic_builder<conan_spec, conan>
{

    using basic_builder<conan_spec, conan>::create;

    static constexpr auto subdir = "/generators"sv;

    enum class build_types
    {
        UNKNOWN = -1,
        Debug,
        Release,
        RelWithDebInfo
    };

    static constexpr auto all_types = std::array {
        std::pair { build_types::Debug, "Debug"sv }, 
        std::pair { build_types::Release, "Release"sv }, 
        std::pair { build_types::RelWithDebInfo, "RelWithDebInfo"sv}
    };

    static constexpr build_types from(std::string_view name)
    {
        for (auto [type, type_name] : all_types) {
            if (name == type_name) {
                return type;
            }
        }
        return UNKNOWN;
    }

    conan_profile current_profile;

    using enum build_types;

    constexpr friend auto to_string(build_types type)
    {
        switch (type) {
        case Debug:
            return "Debug"s;
        case Release:
            return "Release"s;
        case RelWithDebInfo:
            return "RelWithDebInfo"s;
        default:
            return "Debug"s;
        }
    }

    static constexpr auto env_script(build_types type)
    {
        auto script = "conanbuildenv-"s + to_string(type);
        std::ranges::transform(script, std::begin(script), [](auto c) { return std::tolower(c); });
        return script;
    }

    static inline constexpr auto profiles = std::array{ conan_profile{ "gcc:build/gcc/generators"sv },
                                                        conan_profile{ "clang:build_clang/clang/generators"sv } };

    constexpr static auto find_profile(std::string_view profile)
    {
        if (auto build_dir_it = std::ranges::find(profiles, profile, &conan_profile::name);
            build_dir_it != std::end(profiles)) {
            return *build_dir_it;
        } else {
            return profiles.front();
        }
    }

    conan(work_dir dir, env::environment::optional env_)
        : basic_builder{ dir, env_.has_value() ? env_ : env::environment{} }
        , current_profile{ find_profile("CURRENT_PROFILE"_env.value_or("gcc")) }
    {
        auto& environ                  = env();
        environ.set("CURRENT_PROFILE") = current_profile.name;
        environ.set("BUILD_DIR")       = current_profile.build_dir;
        environ.import("PATH");
        environ.import("LD_LIBRARY_PATH");
    }

    bool is_needed(conan_profile profile, build_types type) const
    {
        auto generator_path = working_directory() / fs::path{ profile.build_dir } / fs::path{ profile.generator_dir };
        auto build_script   = env_script(type);

        if (!fs::is_directory(generator_path)) {
            return true;
        }

        for (auto entry : fs::directory_iterator(generator_path)) {
            auto filename = entry.path().filename().string();
            if (filename.starts_with(build_script)) {
                return false;
            }
        }
        return true;
    }

    execution_result execute_step(execution_result result, conan_profile profile, build_types type) const
    {
        return execution_result::merge(
            result,
            root().execute(
                command,
                std::array{ "install"s,
                            "--build=missing"s,
                            "--profile:all="s + std::string{ profile.name },
                            "-s"s,
                            "build_type="s + to_string(type),
                            root().path().string() },
                env()));
    }

    execution_result execute(std::string_view) const override // Target is ignored in this step.
    {
        execution_result result = execution_result::NOT_NEEDED;
        for (auto profile : profiles) {
            for (auto build_type : { Debug, Release, RelWithDebInfo }) {
                if (is_needed(profile, build_type)) {
                    std::println("Profile {} / Build {}", profile.name, to_string(build_type));
                    result = execute_step(result, profile, build_type);
                }
            }
        }
        return result;
    }

    builder_base::ptr next_builder() const override { return cmake::create(root(), env()); }
};

};

#endif // INCLUDED_CONAN_HPP
