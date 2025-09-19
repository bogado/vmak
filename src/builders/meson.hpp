
#ifndef INCLUDED_MESON_HPP
#define INCLUDED_MESON_HPP

#include "builder.hpp"
#include "ninja.hpp"

#include <filesystem>
namespace vb::maker::builders {

struct meson_spec
{
    static constexpr auto stage      = task_type::configuration;
    static constexpr auto name       = "Meson"sv;
    static constexpr auto build_file = "meson.build"sv;
    static constexpr auto command    = "meson"sv;
    static constexpr auto build_dir  = true;
    static constexpr auto creates = builder_base::COMPILE_COMMANDS;
};

struct meson : basic_builder<meson_spec, meson>
{
    using basic_builder<meson_spec, meson>::create;

    meson(work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
    {}

private:
    arguments_type get_arguments(std::string_view) const override  {
        return arguments_builder("setup", get_build_dirname(environment()));
    }

    // TODO: This is duplicate code.
    builder_base::ptr get_next_builder() const override
    {
        auto new_env = environment();
        return builder_base::ptr{std::make_unique<ninja>(root(), environment(), get_build_dir())};
    }
};

}
#endif // INCLUDED_MESON_HPP
