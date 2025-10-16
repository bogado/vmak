#ifndef INCLUDED_CMAKE_PRESET_HPP
#define INCLUDED_CMAKE_PRESET_HPP

#include "../builder.hpp"
#include "../tasks.hpp"
#include "../work_directory.hpp"
#include <bits/utility.h>
#include <nlohmann/json.hpp>
#include <util/environment.hpp>

#include <filesystem>
#include <flat_map>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using json = nlohmann::json;

namespace vb::maker::builders {

struct cmake_preset_spec
{
    static constexpr auto stage      = task_type::DYNAMIC;
    static constexpr auto stages     = std::array{ task_type::configuration, task_type::build, task_type::test };
    static constexpr auto name       = "cmake - preset"sv;
    static constexpr auto build_file = std::array{ "CMakeUserPresets.json"sv, "CMakePresets.json"sv };
    static constexpr auto command    = "cmake"sv;
};

using preset_type = std::string;

using namespace std::literals;

class presets_storage
{
public:

    friend class cmake_preset;
    using storage = std::vector<preset_type>;

private:

    static constexpr auto valid_types =
        std::array{ task_type::configuration, task_type::build, task_type::test, task_type::package };
    static constexpr auto preset_keys =
        std::array{ "configurePresets"sv, "buildPresets"sv, "testPresets"sv, "packagePreset"sv};

    std::array<std::vector<preset_type>, std::to_underlying(task_type::DONE)> presets;

    static constexpr auto locate(task_type type)
    {
        if (auto it = std::ranges::find(valid_types, type); it != valid_types.end()) {
            return static_cast<std::size_t>(std::distance(valid_types.begin(), it));
        } else {
            throw std::domain_error{ std::format("no such type of cmake preset: {}\n", type) };
        }
    }

    static constexpr auto key_of(task_type type)
    {
        return preset_keys.at(locate(type));
    }

    void load_presets(std::filesystem::path file_path);

public:

    auto view_for(task_type type) const
    {
        return std::views::all(presets.at(locate(type)));
    }

    auto appender_for(std::size_t index)
    {
        return std::back_inserter(presets.at(index));
    }

    presets_storage(std::same_as<std::filesystem::path> auto... files)
        requires(sizeof...(files) >= 1)
        : presets{}
    {
        auto all_files = std::vector{ files... };

        for (auto file : all_files) {
            load_presets(file);
        }
    }
};

inline void presets_storage::load_presets(std::filesystem::path file_path)
{
    if (!std::filesystem::is_regular_file(file_path)) {
        return;
    }

    auto content = json::parse(std::ifstream{ file_path });
    if (content.contains("include")) {
        for (const auto& [_, path_json] : content["include"].items()) {
            if (path_json.is_string()) {
                auto include_file = file_path.parent_path() / path_json.get<std::filesystem::path>();
                load_presets(include_file);
            }
        }
    }

    for (auto [index, key] : preset_keys | std::views::enumerate) {
        auto appender = appender_for(static_cast<std::size_t>(index));
        for (const auto& [_, current] : content[key].items()) {
            *appender = current["name"].get<std::string>();
        }
    }
}

class cmake_preset : public basic_builder<cmake_preset_spec, cmake_preset>
{
public:
    using parent = basic_builder<cmake_preset_spec, cmake_preset>;
    using enum task_type;

private:

    presets_storage my_presets;
    task_type       my_task = configuration;

    static auto all_build_files()
    {
        return std::ranges::to<std::vector>(
            build_file | std::views::transform([](const auto& filename) { return std::filesystem::path{ filename }; }) |
            std::views::filter([](auto path) { return std::filesystem::is_regular_file(path); }));
    }

    constexpr Stage get_stage() const override
    {
        return Stage{my_task};
    }

public:

    using basic_builder<cmake_preset_spec, cmake_preset>::create;

    cmake_preset(work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
        , my_presets{ wd.path() / build_file[0], wd.path() / build_file[1] }
    {
    }

    cmake_preset(task_type current_stage, work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
        , my_presets{ wd.path() / build_file[0], wd.path() / build_file[1] }
        , my_task{ current_stage }
    {
    }

    auto presets_for(task_type type) const
    {
        return my_presets.view_for(type);
    }

private:

    arguments_type get_arguments(std::string_view target) const override
    {
        auto preset = target;

        if (preset.empty()) {
            preset = my_presets.view_for(my_task).front();
        }

        auto result = arguments_type{};
        auto append = [&](auto args) {
            std::ranges::copy(
                args | std::views::transform([](auto view) { return std::string{ view }; }),
                std::back_inserter(result));
        };
        switch (my_task) {
        case task_type::configuration:
            append(std::array{ "--preset"sv, preset });
            break;
        case task_type::build:
            append(std::array{ "--build"sv, "--preset"sv, preset });
            break;
        case task_type::test:
            append(std::array{ "--output-on-failure"sv, "--preset"sv, preset });
            break;
        default:
            break;
        }
        return result;
    }

    std::string_view get_command(std::string_view) const override
    {
        if (my_task == task_type::test) {
            return "ctest"sv;
        } else {
            return cmake_preset_spec::command;
        }
    }

    builder_base::ptr get_next_builder() const override
    {
        auto next_task = task_type::DONE;
        switch (my_task) {
        case task_type::configuration:
            next_task = task_type::build;
            break;
        case task_type::build:
            next_task = task_type::test;
        default:
            break;
        }

        if (next_task == task_type::DONE) {
            return {};
        }
        return std::make_unique<cmake_preset>(next_task, root(), environment());
    }

    std::string get_name() const override
    {
        static constexpr auto name = "cmake → preset";
        if (my_task != task_type::DONE) {
            return std::format("{} «{}»", name, my_task);
        }
        return name;
    }
};

} // namespace vb::makers::

#endif // INCLUDED_CMAKE_PRESET_HPP
