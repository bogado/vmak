#ifndef INCLUDE_MAK_WORK_DIRECTORY_HPP_
#define INCLUDE_MAK_WORK_DIRECTORY_HPP_

#include "./result.hpp"
#include "util/environment.hpp"
#include "util/filesystem.hpp"
#include <util/execution.hpp>

#include <concepts>
#include <filesystem>
#include <ranges>
#include <vector>

namespace vb::maker {

using namespace std::literals;

using arguments = std::vector<std::string_view>;

template<typename WORKDIR>
concept is_work_directory = requires(const WORKDIR dir) {
    { dir.path() } -> std::same_as<fs::path>;
    { dir.has_file(std::string{ "filename" }) } -> std::same_as<bool>;
    { dir.has_folder(std::string{ "filename" }) } -> std::same_as<bool>;
    { dir.folder() } -> std::same_as<fs::path>;
    { dir.execute("command"sv, arguments{}) } -> std::convertible_to<bool>;
    { dir.execute("command"sv, env::environment::optional{}) } -> std::convertible_to<bool>;
    { dir.execute("command"sv, arguments{}, env::environment{}) } -> std::convertible_to<bool>;
};

struct work_dir
{
    fs::path root;

    explicit work_dir(fs::path rt = fs::current_path())
        : root{ rt }
    {
    }

    fs::path path() const
    {
        return root;
    }

    bool has_file(std::string_view file) const
    {
        return fs::is_regular_file(root / file);
    }

    bool has_folder(std::string_view file) const
    {
        return fs::is_directory(root / file);
    }

    fs::path folder(std::string_view sub_folder) const
    {
        auto result = (root / sub_folder);
        if (!has_folder(sub_folder)) {
            std::error_code code;
            if (!fs::create_directories(result, code)) {
                throw std::system_error(code, std::format("Could not create build directory {}", result));
            }
        }
        return result;
    }

    auto execute(std::string_view command, std::ranges::contiguous_range auto args, env::environment::optional env = {})
        const -> execution_result
    {
        auto errors = std::vector<std::string>{};
        std::print(" {} [ {} ", root.string(), command);
        for (const auto& arg : args) {
            std::print("«{}» ", arg);
        };
        std::println(" ]\n");

        execution executer{ io_set::ERR };
        executer.execute(
            command,
            std::ranges::to<std::vector>(args | std::views::transform([](auto view) {
                                             return std::string(view);
                                         })),
            env,
            root);

        return execution_result{ executer };
    }
};

}

#endif // INCLUDE_MAK_WORK_DIRECTORY_HPP_
