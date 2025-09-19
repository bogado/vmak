#ifndef INCLUDED_CONAN_HPP
#define INCLUDED_CONAN_HPP

#include "../builder.hpp"
#include "../builders/cmake_preset.hpp"
#include "json.hpp"
#include "tasks.hpp"
#include <unistd.h>
#include <util/environment.hpp>
#include <util/execution.hpp>
#include <util/string_list.hpp>
#include <util/system.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <concepts>
#include <filesystem>
#include <format>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace vb::maker::builders {

using namespace env::literals;

struct conan_spec
{
    static constexpr auto stage      = task_type::pre_requisites;
    static constexpr auto name       = "conan"sv;
    static constexpr auto build_file = std::array{"conanfile.txt"sv, "conanfile.py"sv};
    static constexpr auto command    = "conan"sv;
};

struct conan_invocation_error : std::runtime_error
{
private:

    static std::string
    make_what(std::integral auto rc, const std::ranges::range auto& args, const std::ranges::range auto& stderr)
    {
        return std::format(
            "Conan invocation failed with rc={} for :\n"
            "┌─> {}\n"
            "│{}",
            rc,
            std::ranges::to<std::string>(
                args | std::views::transform([](const auto& arg) {
                    return "'"s + std::string{ arg } + "'"s;
                }) |
                std::views::join_with(" "s)),
            std::ranges::to<std::string>(stderr | std::views::join_with("│"s)));
    }

public:

    conan_invocation_error(
        std::integral auto             error_code,
        std::ranges::range auto&&      args,
        const std::ranges::range auto& stderr)
        : std::runtime_error{ make_what(error_code, args, stderr).c_str() }
    {
    }
};

namespace details {

using config_dump = std::unordered_map<std::string, std::optional<std::string>>;

static auto run_conan(std::filesystem::path configuration, std::convertible_to<std::string_view> auto... args)
    -> std::vector<std::string>
{
    static constexpr auto command    = "conan"sv;
    static constexpr auto conan_home = "CONAN_HOME"_env;

    if (!configuration.empty()) {
        env::variable(conan_home).set(std::filesystem::absolute(configuration).native());
    }

    const auto arguments = std::array{ std::string_view{ args }... };

    execution conan{ io_set::OUT | io_set::ERR };

    conan.execute(command, arguments);

    auto result =
        std::ranges::to<std::vector>(conan.lines<std_io::OUT>() | std::views::transform([](const std::string& line) {
                                         static constexpr auto skip  = " \t\n\r";
                                         auto                  start = line.find_first_not_of(skip);
                                         start                       = start == line.npos ? 0 : start;
                                         auto last                   = line.find_last_not_of(skip);
                                         auto size = last == line.npos ? line.size() - start : last - start;
                                         return line.substr(start, size + 1);
                                     }));
    if (auto error = conan.wait(); error != 0) {
        auto stderr = std::ranges::to<std::vector>(conan.lines<std_io::ERR>());
        throw conan_invocation_error{ error, std::array{ args... }, stderr };
    }

    return result;
}

static auto run_conan(std::convertible_to<std::string_view> auto... args)
{
    return run_conan(std::filesystem::path{}, args...);
}

static config_dump run_conan_json(
    std::filesystem::path configuration,
    std::convertible_to<std::string_view> auto... args)
{
    auto parsed = nlohmann::json::parse(
                      std::ranges::to<std::string>(
                          run_conan(configuration, args..., "--format"sv, "json"sv) | std::views::join_with(" "s)))
                      .flatten();

    auto result = config_dump{};

    for (auto [key, value] : parsed.items()) {
        result.emplace(
            std::pair{ key, !value.is_null() ? std::optional{ value.template get<std::string>() } : std::nullopt });
    }
    return result;
}

static auto run_conan_json(std::convertible_to<std::string_view> auto... args)
{
    return run_conan_json(std::filesystem::path{}, args...);
}

struct conan_configuration
{
    using storage = details::config_dump;

private:

    storage my_data;

public:

    explicit conan_configuration(
        std::filesystem::path root,
        std::string_view      command,
        const std::convertible_to<std::string_view> auto&...args)
        : my_data{ details::run_conan_json(root, command, std::string_view{ args }...) }
    {
    }

    explicit conan_configuration(std::string_view command, const std::convertible_to<std::string_view> auto&...args)
        : conan_configuration{ std::filesystem::path{}, command, args... }
    {
    }

    static conan_configuration
    from_profile(std::filesystem::path root, std::string_view build, std::string_view host = {})
    {
        return conan_configuration{
            root, "profile"sv, "show"sv, "--profile:build"sv, build, "--profile:host"sv, host.empty() ? build : host
        };
    }

    static conan_configuration from_profile(std::string_view build, std::string_view host = {})
    {
        return from_profile(std::filesystem::path{}, build, host);
    }

    explicit conan_configuration(std::filesystem::path root)
        : conan_configuration{ root, "config"sv, "show", "*" }
    {
    }

    conan_configuration()
        : conan_configuration{ std::filesystem::path{} }
    {
    }

    std::string operator[](const std::convertible_to<std::string_view> auto&...keys) const
    {
        const auto all = std::array{ std::string_view{ keys }... };
        const auto key = std::ranges::to<std::string>(all | std::views::join_with("/"sv));
        if (auto it = my_data.find(key); it != std::end(my_data)) {
            return it->second.value_or(""s);
        } else {
            return ""s;
        }
    }

    auto size() const
    {
        return my_data.size();
    }

    auto keys() const
    {
        return my_data | std::views::keys;
    }

    auto values() const
    {
        return my_data | std::views::values;
    }
};
}

struct conan_profile
{
private:

    constexpr std::string_view storage_view()
    {
        if (!storage) {
            return {};
        }

        return *storage;
    }

public:

    constexpr explicit conan_profile(std::string_view repr)
        : storage{}
        , name{ repr.substr(0, repr.find(':')) }
        , build_dir{ repr.substr(name.size() + 1, repr.find('/') - name.size() - 1) }
        , generator_dir{ repr.substr(name.size() + build_dir.size() + 2) }
    {
        if !consteval {
            std::abort();
        }
    }

    explicit conan_profile(std::string arg_name, std::string arg_build_dir, std::string arg_generator_dir)
        : storage{ arg_name + arg_build_dir + arg_generator_dir }
        , name{ storage_view().substr(0, arg_name.size()) }
        , build_dir{ storage_view().substr(name.size(), arg_build_dir.size()) }
        , generator_dir{ storage_view().substr(name.size() + arg_build_dir.size()) }
    {
        if consteval {
            throw std::logic_error("Invalid constexpr initialization");
        }
    }

    std::optional<std::string> storage{};
    std::string_view           name;
    std::string_view           build_dir;
    std::string_view           generator_dir;
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

    static constexpr auto all_types = std::array{ std::pair{ build_types::Debug, "Debug"sv },
                                                  std::pair{ build_types::Release, "Release"sv },
                                                  std::pair{ build_types::RelWithDebInfo, "RelWithDebInfo"sv } };

    friend constexpr build_types operator++(build_types type)
    {
        auto keys = all_types | std::views::keys;
        auto it   = std::ranges::find(keys, type);
        if (it == std::end(keys) || std::next(it) == std::end(keys)) {
            return UNKNOWN;
        }

        return *std::next(it);
    }

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
    build_types   current_build_type;

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
        std::ranges::transform(script, std::begin(script), [](auto c) {
            return std::tolower(c);
        });
        return script;
    }

    static inline auto profiles = std::array{ conan_profile{ "gcc:build/gcc/generators"sv },
                                              conan_profile{ "clang:build_clang/clang/generators"sv } };

    constexpr static const auto& find_profile(std::string_view profile)
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
        , current_build_type(all_types.front().first)
    {
        environment().set("CURRENT_PROFILE") = current_profile.name;
        environment().set("BUILD_DIR")       = current_profile.build_dir;
        environment().import("PATH");
        environment().import("LD_LIBRARY_PATH");
    }

    bool is_needed(conan_profile profile, build_types type) const
    {
        auto generator_path = root().path() / fs::path{ profile.build_dir } / fs::path{ profile.generator_dir };
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

    arguments_type get_arguments(std::string_view) const override
    {
        return std::vector{ "install"s,
                            "--build=missing"s,
                            "--profile:all="s + std::string{ current_profile.name },
                            "-s"s,
                            "build_type="s + to_string(current_build_type),
                            root().path().string() };
    }

    builder_base::ptr get_next_builder() const override
    {
        return std::make_unique<cmake_preset>(task_type::configuration, root(), environment());
    }
};

} // namespace

template<>
struct std::formatter<vb::maker::builders::details::conan_configuration>
{
    template<class PARSE_CONTEXT>
    constexpr PARSE_CONTEXT::iterator parse(PARSE_CONTEXT& context)
    {
        return context.begin();
    }

    template<class FORMAT_CONTEXT>
    constexpr FORMAT_CONTEXT::iterator format(
        const vb::maker::builders::details::conan_configuration& configuration,
        FORMAT_CONTEXT&                                          context) const
    {
        auto out = context.out();
        return std::ranges::copy(
                   configuration.keys() | std::views::transform([&](const auto& key) {
                       const auto quote = "\""s;
                       auto       value = configuration[key];
                       return quote + key + "\" : "s + (!value.empty() ? quote + value + quote : "null"s);
                   }) | std::views::join_with(", "s),
                   out)
            .out;
    }
};

template<>
struct std::formatter<vb::maker::builders::conan_profile>
{
    template<class PARSE_CONTEXT>
    constexpr PARSE_CONTEXT::iterator parse(PARSE_CONTEXT& context)
    {
        return context.begin();
    }

    template<class FORMAT_CONTEXT>
    constexpr FORMAT_CONTEXT::iterator format(
        const vb::maker::builders::conan_profile& profile,
        FORMAT_CONTEXT&                           context) const
    {
        auto out = context.out();
        out      = std::ranges::copy(profile.name, out).out;
        *out     = ':';
        ++out;
        out  = std::ranges::copy(profile.build_dir, out).out;
        *out = '/';
        ++out;
        out = std::ranges::copy(profile.generator_dir, out).out;
        return out;
    }
};

#endif // INCLUDED_CONAN_HPP
