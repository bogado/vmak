#ifndef INCLUDED_CMAKE_PRESET_HPP
#define INCLUDED_CMAKE_PRESET_HPP

#include "../builder.hpp"
#include "../presets.hpp"
#include <nlohmann/json.hpp>
#include <util/environment.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using json = nlohmann::json;

namespace vb::maker::builders {

struct cmake_preset_spec
{
    static constexpr std::string_view name        = "cmake - preset";
    static constexpr auto             build_files = std::array{ "CMakeUserPresets.json"sv, "CMakePresets.json"sv };
    static constexpr std::string_view command     = "cmake";
};

using preset = std::string;

using namespace std::literals;

class presets_storage
{
public:
    friend class cmake_preset;

    auto view_for(preset_type type) const
    {
        switch (type) {
        case preset_type::configuration:
            return std::views::all(configure);
        case preset_type::build:
            return std::views::all(build);
        case preset_type::test:
            return std::views::all(test);
        }
        throw std::logic_error(std::format("Invalid type {}.", type));
    }

    auto appender_for(preset_type type) 
    {
        switch (type) {
        case preset_type::configuration:
            return std::back_inserter(configure);
        case preset_type::build:
            return std::back_inserter(build);
        case preset_type::test:
            return std::back_inserter(test);
        }
        throw std::logic_error(std::format("Invalid type {}.", type));
    }

private:
    static constexpr auto key_of(preset_type type)
    {
        static auto keys = std::array{ "configurePresets"sv, "buildPresets"sv, "testPresets"sv };
        return keys[static_cast<std::size_t>(type)];
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

        for (auto type : { preset_type::configuration, preset_type::build, preset_type::test }) {
            if (content.contains(key_of(type))) {
                auto appender = appender_for(type);
                for (const auto& [_, current] : content[key_of(type)].items()) {
                    *appender = current["name"].get<std::string>();
                }
            }
        }
    }

    std::vector<preset> configure;
    std::vector<preset> build;
    std::vector<preset> test;

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
    using enum preset_type;

private:
    presets_storage my_presets;

    static auto all_build_files()
    {
        return std::ranges::to<std::vector>(
            build_file | std::views::transform([](const auto& filename) { return std::filesystem::path{ filename }; }) |
            std::views::filter([](auto path) { return std::filesystem::is_regular_file(path); }));
    }

public:
    using basic_builder<cmake_preset_spec, cmake_preset>::create;

    cmake_preset(work_dir wd, env::environment::optional env_)
        : basic_builder{ wd, env_ }
        , my_presets{ wd.path() / build_file[0], wd.path() / build_file[1] }
    {
    }

    execution_result execute(std::string_view target) const override
    {
        using namespace std::literals;
        auto contains = [](const auto& collection, const auto& what) {
            return std::ranges::find(collection, what) != std::end(collection);
        };

        std::string target_str = std::string{ target };
        if (contains(my_presets.configure, target)) {
            return root().execute(command, std::array{ "--profile"s, target_str }, env());
        } else if (contains(my_presets.build, target)) {
            return root().execute(command, std::array{ "--build"s, "--profile"s, target_str }, env());
        } else if (contains(my_presets.build, target)) {
            return root().execute(command, std::array{ "--test"s, "--profile"s, target_str }, env());
        }
        return execution_result{ std::format("Could not find preset {}", target) };
    }

    auto presets_for(preset_type type) const { return my_presets.view_for(type); }
};

}

#endif // INCLUDED_CMAKE_PRESET_HPP
