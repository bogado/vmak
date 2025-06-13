#ifndef INCLUDED_BUILDER_HPP
#define INCLUDED_BUILDER_HPP

#include "./result.hpp"
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
    using ptr     = std::unique_ptr<builder_base>;
    using factory = ptr (*)(work_dir, env::environment::optional);

    virtual ~builder_base() = default;

    virtual std::string name() const     = 0;
    virtual work_dir    root() const     = 0;
    virtual bool        required() const = 0;
    virtual ptr         next_builder() const { return nullptr; }

    virtual fs::path         working_directory() const { return root().path(); }
    virtual execution_result execute(std::string_view target) const = 0;
};

template<typename BUILDER>
concept is_builder = is_runner<BUILDER> && std::derived_from<builder_base, BUILDER> && requires(const fs::path path) {
    { BUILDER::is_root(path) } -> std::same_as<bool>;
};

template<typename SPEC_T>
concept is_builder_spec = requires {
    requires is_string<decltype(SPEC_T::name)>;
    requires is_string<decltype(SPEC_T::build_file)> ||
                 (std::ranges::range<decltype(SPEC_T::build_files)> &&
                  std::same_as<std::ranges::range_value_t<decltype(SPEC_T::build_files)>, std::string_view>);
    requires is_string<decltype(SPEC_T::command)>;
};

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
        if constexpr (requires {
                          { SPECIFICATION::build_files };
                      }) {
            return std::ranges::any_of(SPECIFICATION::build_files, [&](const auto& filename) {
                                           return root.has_file(filename);
                                       });
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

    static constexpr auto command    = SPECIFICATION::command;
    static constexpr auto build_file = []() {
        if constexpr (requires {
                          { SPECIFICATION::build_file };
                      }) {
            return SPECIFICATION::build_file;
        } else {
            return SPECIFICATION::build_files;
        }
    }();

    explicit basic_builder(work_dir rt, env::environment::optional env_, std::same_as<std::string_view> auto... args)
        : my_root{ rt }
        , my_env{ env_.value_or(env::environment{}) }
        , my_args{ args... }
    {
        if constexpr (requires { SPECIFICATION::import_vars; }) {
            for (auto var_name : SPECIFICATION::import_vars) {
                env().import(var_name);
            }
        }
        for (auto var_name : std::array{ "HOME", "USER", "USERNAME", "UID", "PATH" }) {
            env().import(var_name);
        }
    }

    std::string name() const override { return vb::to_string(SPECIFICATION::name); }

    work_dir root() const override { return my_root; }

    basic_builder::ptr next_builder() const override { return nullptr; }

    bool required() const override { return true; }

    auto run(std::string_view target) { return execute(arguments(target)); }

    std::vector<std::string> arguments_builder(std::same_as<std::string_view> auto... args) const
    {
        std::vector<std::string> result;
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

    virtual std::vector<std::string> arguments(std::string_view target) const
    {
        if (target.empty()) {
            return arguments_builder();
        } else {
            return arguments_builder(target);
        }
    }

    execution_result execute(std::string_view target = {}) const override
    {
        std::string target_arg;
        if constexpr (requires { SPECIFICATION::target_argument_prefix; }) {
            target_arg = std::string(SPECIFICATION::target_argument_prefix) + std::string{ target };
        } else {
            target_arg = target;
        }

        return root().execute(command, arguments(target_arg), env());
    }

protected:
    const auto& env() const { return my_env; }

    auto& env() { return my_env; }
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

    explicit operator bool() const { return impl != nullptr; }

    std::string name() const override
    {
        if (!*this) {
            return "«NO BUILDER»";
        }

        return impl->name();
    }

    bool required() const override
    {
        if (!*this) {
            return false;
        }

        return impl->required();
    }

    execution_result execute(std::string_view target = {}) const override
    {
        if (!*this) {
            return execution_result::PANIC;
        }

        return impl->execute(target);
    }

    work_dir root() const override
    {
        if (!*this) {
            return work_dir{};
        }

        return impl->root();
    }

    ptr next_builder() const override
    {
        if (!*this) {
            return nullptr;
        }

        return impl->next_builder();
    }

    builder next() const
    {
        if (*this) {
            if (auto nxt = impl->next_builder(); nxt != nullptr) {
                return builder{ std::move(nxt) };
            }
        }
        return builder{};
    }

    fs::path working_directory() const override { return impl->working_directory(); }
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
