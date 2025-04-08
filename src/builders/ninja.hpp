#ifndef INCLUDED_NINJA_HPP
#define INCLUDED_NINJA_HPP

#include "../builder.hpp"

#include <filesystem>
#include <string>

namespace vb::maker::builders {

using namespace std::literals;
using namespace vb::literals;

struct ninja_spec
{
    static constexpr std::string_view name        = "ninja";
    static constexpr std::string_view build_file  = "build.ninja";
    static constexpr std::string_view command     = "ninja";
    static constexpr auto             import_vars = std::array{ "BUILD_TARGET", "NINJA_FILE" };
};

struct ninja : basic_builder<ninja_spec, ninja>
{
    using basic_builder<ninja_spec, ninja>::create;

    static constexpr auto BUILD_FILE_VAR = "NINJA_FILE";

    fs::path build_file() const
    {
        if (auto var = env().get(BUILD_FILE_VAR); var.has_value() && var.value().has_value()) {
            return fs::path{ var.value().value_str() };
        }
        return fs::path{};
    }

    ninja(work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_.has_value() ? env_ : env::environment{} }
    {
        env().import(BUILD_FILE_VAR);
    }

    std::vector<std::string> arguments(std::string_view target) const override
    {
        auto args = std::vector<std::string>{};
        if (!build_file().empty() && fs::exists(work_dir().path() / build_file())) {
            args.push_back("-f"s);
            args.push_back(work_dir().path() / build_file());
        }
        if (!target.empty()) {
            args.push_back(std::string{ target });
        }
        return args;
    }
};

}

#endif // INCLUDED_NINJA_HPP
