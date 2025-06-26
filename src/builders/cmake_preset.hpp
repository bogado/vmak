#ifndef INCLUDED_CMAKE_PRESET_HPP
#define INCLUDED_CMAKE_PRESET_HPP

#include "../builder.hpp"
#include "../tasks.hpp"
#include "work_directory.hpp"
#include <nlohmann/json.hpp>
#include <util/environment.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <flat_map>
#include <vector>
#include <format>

using json = nlohmann::json;

namespace vb::maker::builders {

struct cmake_preset_spec
{
    static constexpr std::string_view name        = "cmake - preset";
    static constexpr auto             build_files = std::array{ "CMakeUserPresets.json"sv, "CMakePresets.json"sv };
    static constexpr std::string_view command     = "cmake";
};

using preset_type = std::string;

using namespace std::literals;

class presets_storage
{
    friend class cmake_preset;

    auto view_for(task_type type) const
    {
        switch (type) {
        case task_type::pre_requisites:
            [[fallthrough]];
        case task_type::configuration:
            return std::views::all(configure);
        case task_type::build:
            return std::views::all(build);
        case task_type::test:
            return std::views::all(test);
        case task_type::DONE:
            throw std::runtime_error("Done");
            
        }
        throw std::logic_error(std::format("Invalid type {}.", type));
    }

    auto appender_for(task_type type)
    {
        switch (type) {
        case task_type::pre_requisites:
            [[fallthrough]];
        case task_type::configuration:
            return std::back_inserter(configure);
        case task_type::build:
            return std::back_inserter(build);
        case task_type::test:
            return std::back_inserter(test);
        case task_type::DONE:
            throw std::runtime_error("Done");
        }
        throw std::logic_error(std::format("Invalid type {}.", type));
    }

private:
    static constexpr auto key_of(task_type type)
    {
        static const auto keys = std::flat_map<task_type, std::string_view>{
            { task_type::configuration, "configurePresets"sv },
            { task_type::build, "buildPresets"sv },
            { task_type::test, "testPresets"sv }
        };
        if (auto found = keys.find(type); found != std::end(keys)) {
            return found->second;
        } else {
            throw std::domain_error{std::format("no such type of preset: {}\n", type)};
        }
    }

    auto load_presets(std::filesystem::path file_path)
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

        for (auto type : { task_type::configuration, task_type::build, task_type::test }) {
            if (content.contains(key_of(type))) {
                auto appender = appender_for(type);
                for (const auto& [_, current] : content[key_of(type)].items()) {
                    *appender = current["name"].get<std::string>();
                }
            }
        }
    }

    std::vector<preset_type> configure;
    std::vector<preset_type> build;
    std::vector<preset_type> test;

public:
    presets_storage(std::same_as<std::filesystem::path> auto... files)
        requires(sizeof...(files) >= 1)
        : configure{}
        , build{}
        , test{}
    {
        auto all_files = std::vector{ files... };

        for (auto file : all_files) {
            load_presets(file);
        }
    }
};

class cmake_preset : public basic_builder<cmake_preset_spec, cmake_preset>
{
public:
    using enum task_type;

private:
    presets_storage my_presets;
    task_type my_task = DONE;

    static auto all_build_files()
    {
        return std::ranges::to<std::vector>(
            build_file | std::views::transform([](const auto& filename) { return std::filesystem::path{ filename }; }) |
            std::views::filter([](auto path) { return std::filesystem::is_regular_file(path); }));
    }

    auto find_task(std::string_view target) const {
        auto contains = [](const auto& collection, const auto& what) {
            return std::ranges::find(collection, what) != std::end(collection);
        };

        if (contains(my_presets.configure, target)) {
            return task_type::configuration;
        } else if (contains(my_presets.build, target)) {
            return task_type::build;
        } else if (contains(my_presets.build, target)) {
            return task_type::test;
        }
        return my_task;
    }

public:
    using basic_builder<cmake_preset_spec, cmake_preset>::create;

    cmake_preset(work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
        , my_presets{ wd.path() / build_file[0], wd.path() / build_file[1] }
    {
    }

    cmake_preset(task_type task, work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
        , my_presets{ wd.path() / build_file[0], wd.path() / build_file[1] }
        , my_task{task}
    {
    }

    execution_result execute(std::string_view target) const override
    {
        using namespace std::literals;
        auto task = my_task;

        if (target.empty()) {
            target = my_presets.view_for(my_task).front();
        } else {
            task = find_task(target);
        }

        switch (task) {
        case task_type::configuration:
            return root().execute(command, std::array{ "--preset"sv, target }, env());
        case task_type::build:
            return root().execute(command, std::array{ "--build"sv, "--preset"sv, target }, env());
        case task_type::test:
            return root().execute("ctest"sv, std::array{ "--output-on-failure"sv, "--preset"sv, target }, env());
        case task_type::DONE:
            return execution_result{"No action"};
        default:
            return execution_result{ std::format("Could not find preset type {} named {}", task, target) };
        }
    }

    auto presets_for(task_type type) const { return my_presets.view_for(type); }

    builder_base::ptr next_builder() const override
    {
        auto next_task = next(my_task);
        if (next_task == task_type::DONE) {
            return {};
        }
        return std::make_unique<cmake_preset>(next_task, root(), env());
    }

    std::string name() const override {
        static constexpr auto name = "cmake → preset";
        if (my_task != task_type::DONE) {
            return std::format("{} «{}»",name, my_task);
        }
        return name;
    }
};

}

#endif // INCLUDED_CMAKE_PRESET_HPP
