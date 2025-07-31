#ifndef INCLUDED_BUILDER_HPP
#define INCLUDED_BUILDER_HPP

#include "./result.hpp"
#include "./tasks.hpp"
#include "./work_directory.hpp"
#include <util/environment.hpp>
#include <util/filesystem.hpp>
#include <util/string.hpp>

#include <algorithm>
#include <concepts>
#include <memory>
#include <optional>
#include <string_view>

namespace vb::maker {

template<typename RUNNER>
concept is_runner = requires(const RUNNER runner) {
    { runner.working_directory() } -> std::same_as<fs::path>;
    { runner.run() } -> std::same_as<execution_result>;
    { runner.run(std::string_view{}) } -> std::same_as<execution_result>;
};

template<typename OPT_RUNNER>
concept is_optional_runner = std::same_as<std::nullopt_t, OPT_RUNNER> || requires {
    requires is_runner<typename OPT_RUNNER::value_type>;
    requires std::same_as<OPT_RUNNER, std::optional<typename OPT_RUNNER::value_type>>;
};

struct builder_base
{
    builder_base()                               = default;
    builder_base(const builder_base&)            = default;
    builder_base(builder_base&&)                 = default;
    builder_base& operator=(const builder_base&) = default;
    builder_base& operator=(builder_base&&)      = default;

    friend struct builder;

    using ptr            = std::unique_ptr<builder_base>;
    using factory        = ptr (*)(work_dir, env::environment::optional);
    using target_type    = std::string_view;
    using arguments_type = std::vector<std::string>;

    virtual ~builder_base() = default;

    constexpr auto name() const
    {
        return get_name();
    }

    constexpr auto root() const
    {
        return get_root();
    }

    constexpr auto required() const
    {
        return get_required();
    }

    constexpr auto stage() const
    {
        return get_stage();
    }

    constexpr auto next_builder() const
    {
        return get_next_builder();
    }

    constexpr auto run(std::string_view target) const
    {
        return root().execute(get_command(target), get_arguments(target), get_environment(target));
    }

private:

    virtual std::string get_name() const     = 0;
    virtual work_dir    get_root() const     = 0;
    virtual bool        get_required() const = 0;
    virtual task_type   get_stage() const    = 0;
    virtual ptr         get_next_builder() const
    {
        return nullptr;
    }

    virtual std::string_view get_command(std::string_view target [[maybe_unused]]) const = 0;
    

    virtual arguments_type get_arguments(std::string_view target [[maybe_unused]]) const
    {
        return {};
    }

    virtual const env::environment& get_environment(std::string_view target [[maybe_unused]]) const = 0;

    virtual fs::path get_working_directory(std::string_view target [[maybe_unused]]) const
    {
        return root().path();
    }
};

template<typename TYPE, typename VALUE_T>
concept one_or_many =
    std::same_as<std::remove_cvref_t<TYPE>, VALUE_T> ||
    (std::ranges::range<std::remove_cvref_t<TYPE>> && std::same_as<std::ranges::range_value_t<TYPE>, VALUE_T>);

template<typename TYPE>
concept one_or_many_strings = is_string<std::remove_cvref_t<TYPE>> || (std::ranges::range<std::remove_cvref_t<TYPE>> &&
                                                                       is_string<std::ranges::range_value_t<TYPE>>);

template<typename TYPE, typename VALUE_T>
concept is_many_of = std::ranges::range<TYPE> && std::same_as<std::ranges::range_value_t<TYPE>, VALUE_T>;

template<typename BUILDER>
concept is_builder = is_runner<BUILDER> && std::derived_from<builder_base, BUILDER> && requires(const fs::path path) {
    { BUILDER::is_root(path) } -> std::same_as<bool>;
    { BUILDER::task } -> std::same_as<task_type>;
};

template<typename SPEC_T>
concept is_builder_spec = requires {
    { SPEC_T::stage } -> std::convertible_to<task_type>;
    { SPEC_T::name } -> is_string;
    { SPEC_T::command } -> is_string;
    { SPEC_T::build_file } -> one_or_many_strings;
};

template<typename SPEC_T>
concept specification_has_import =
    is_builder_spec<SPEC_T> && std::ranges::range<decltype(SPEC_T::import_env)> &&
    std::convertible_to<std::ranges::range_value_t<decltype(SPEC_T::import_view)>, std::string_view>;

template<is_builder_spec SPECIFICATION, typename CLASS = void>
struct basic_builder : builder_base
{
private:

    work_dir         my_root;
    env::environment my_env{};
    std::string      my_args{};

public:

    static bool is_root(work_dir root)
    {
        if constexpr (is_many_of<decltype(SPECIFICATION::build_file), std::string_view>) {
            return std::ranges::any_of(
                SPECIFICATION::build_file, [&](const auto& filename) { return root.has_file(filename); });
        } else {
            return root.has_file(SPECIFICATION::build_file);
        }
    }

    static constexpr basic_builder::factory create = [](work_dir dir, env::environment::optional env) {
        if (!is_root(dir)) {
            return basic_builder::ptr{ nullptr };
        }

        if constexpr (std::same_as<CLASS, void>) {
            return basic_builder::ptr{ new basic_builder{ dir, env } };
        } else {
            return basic_builder::ptr{ new CLASS{ dir, env } };
        }
    };

    static constexpr auto build_file = SPECIFICATION::build_file;

    explicit basic_builder(work_dir rt, env::environment::optional env_, std::same_as<std::string_view> auto... args)
        : my_root{ rt }
        , my_env{ env_.value_or(env::environment{}) }
        , my_args{ args... }
    {
        if constexpr (requires { SPECIFICATION::import_vars; }) {
            for (auto var_name : SPECIFICATION::import_vars) {
                environment().import(var_name);
            }
        }
        for (auto var_name : std::array{ "HOME"sv,
                                         "USER"sv,
                                         "USERNAME"sv,
                                         "PATH"sv,
                                         "LANG"sv,
                                         "LC_ALL"sv,
                                         "TERM"sv,
                                         "DISPLAY"sv,
                                         "WAYLAND_DISPLAY"sv }) {
            environment().import(var_name);
        }
        if constexpr (specification_has_import<SPECIFICATION>) {
            for (auto var_name : SPECIFICATION::import_env) {
                environment().import(var_name);
            }
        }
    }

private:

    std::string get_name() const override
    {
        return vb::to_string(SPECIFICATION::name);
    }

    work_dir get_root() const override
    {
        return my_root;
    }

    basic_builder::ptr get_next_builder() const override
    {
        return nullptr;
    }

    bool get_required() const override
    {
        return true;
    }

    std::string_view get_command(std::string_view target [[maybe_unused]]) const override
    {
        return SPECIFICATION::command;
    }

    arguments_type arguments_builder(std::convertible_to<std::string_view> auto... args) const
    {
        arguments_type result;
        if constexpr (requires { SPECIFICATION::arguments; }) {
            result.reserve(std::size(SPECIFICATION::arguments) + sizeof...(args));
            std::ranges::copy(
                SPECIFICATION::arguments | std::views::transform([](const auto& arg) { return vb::to_string(arg); }),
                std::back_inserter(result));
        } else {
            result.reserve(sizeof...(args));
        }
        if constexpr (sizeof...(args) > 0) {
            std::ranges::copy(
                std::array{ args... } | std::views::transform([](std::string_view view) { return std::string(view); }),
                std::back_inserter(result));
        }
        return result;
    }

    arguments_type get_arguments(std::string_view target) const override
    {
        std::string target_arg;
        if (!target.empty()) {
            if constexpr (requires { SPECIFICATION::target_argument_prefix; }) {
                target_arg = std::string(SPECIFICATION::target_argument_prefix) + std::string{ target };
            } else {
                target_arg = target;
            }
        }

        return arguments_builder(target_arg);
    }

    const env::environment& get_environment(std::string_view) const override
    {
        return environment();
    }

    task_type get_stage() const override
    {
        return SPECIFICATION::stage;
    }

protected:

    template<typename SELF>
    auto& environment(this SELF&& self)
    {
        return std::forward<SELF>(self).my_env;
    }

    arguments_type arguments(std::string_view target) const
    {
        return get_arguments(target);
    }

    std::string_view command(std::string_view target) const
    {
        return get_command(target);
    }
};

struct builder : builder_base
{
private:

    builder_base::ptr impl;

public:

    using optional = std::optional<builder>;

    constexpr builder() = default;

    builder(builder_base::ptr&& impl_)
        : impl{ std::move(impl_) }
    {
    }

    explicit operator bool() const
    {
        return impl != nullptr;
    }

private:

    std::string get_name() const override
    {
        if (!*this) {
            return "«NO BUILDER»";
        }

        return impl->name();
    }

    task_type get_stage() const override
    {
        if (!*this) {
            return task_type::DONE;
        }
        return impl->stage();
    }

    bool get_required() const override
    {
        if (!*this) {
            return false;
        }

        return impl->required();
    }

    work_dir get_root() const override
    {
        if (!*this) {
            return work_dir{};
        }

        return impl->root();
    }

    ptr get_next_builder() const override
    {
        if (!*this) {
            return nullptr;
        }

        return impl->next_builder();
    }

    const env::environment& get_environment(std::string_view target) const override
    {
        return impl->get_environment(target);
    }

    fs::path get_working_directory(std::string_view target) const override
    {
        return impl->get_working_directory(target);
    }

    std::string_view get_command(std::string_view target) const override
    {
        return impl->get_command(target);
    }
};
} // namespace vb::maker

template<>
struct std::formatter<vb::maker::builder, char>
{
    template<class PARSE_CONTEXT>
    constexpr PARSE_CONTEXT::iterator parse(PARSE_CONTEXT& context)
    {
        return context.begin();
    }

    template<class FORMATTER_CONTEXT>
    FORMATTER_CONTEXT::iterator format(const vb::maker::builder& builder, FORMATTER_CONTEXT& context) const
    {
        auto out = context.out();
        out      = std::ranges::copy("🔧 "sv, out).out;
        out      = std::ranges::copy(builder.name(), context.out()).out;
        return out;
    }
};

#endif // INCLUDED_BUILDER_HPP
