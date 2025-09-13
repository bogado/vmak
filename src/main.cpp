#include "arguments.hpp"
#include "builders.hpp"
#include "tasks.hpp"
#include <util/converters.hpp>
#include <util/environment.hpp>
#include <util/options.hpp>

#include <filesystem>
#include <print>
#include <ranges>
#include <string_view>
#include <vector>

using namespace vb;
using namespace std::literals;

constexpr auto default_environment = std::array{
    "CC"sv,
    "CXX"sv,
    "DISPLAY"sv,
    "GID"sv,
    "HOME"sv,
    "HOST"sv,
    "HOSTNAME"sv,
    "LANG"sv,
    "LC_ADDRESS"sv,
    "LC_ALL"sv,
    "LC_ALL"sv,
    "LC_MEASUREMENT"sv,
    "LC_MONETARY"sv,
    "LC_NUMERIC"sv,
    "LC_PAPER"sv,
    "LC_TELEPHONE"sv,
    "LC_TIME"sv,
    "PATH"sv,
    "SHELL"sv,
    "SSH_AGENT_PID"sv,
    "SSH_AUTO_SOCK"sv,
    "TERM"sv,
    "TTY"sv,
    "UID"sv,
    "USER"sv,
    "USERNAME"sv,
    "WAYLAND_DISPLAY"sv,
    "XDG_CACHE_HOME"sv,
    "XDG_CONFIG_DIRS"sv,
    "XDG_CONFIG_HOME"sv,
    "XDG_DATA_DIRS"sv,
    "XDG_DATA_HOME"sv,
    "XDG_DESKTOP_DIR"sv,
    "XDG_DOCUMENTS_DIR"sv,
    "XDG_DOWNLOAD_DIR"sv,
    "XDG_MUSIC_DIR"sv,
    "XDG_PICTURES_DIR"sv,
    "XDG_PUBLICSHARE_DIR"sv,
    "XDG_RUNTIME_DIR"sv,
    "XDG_STATE_HOME"sv,
    "XDG_TEMPLATES_DIR"sv,
    "XDG_VIDEOS_DIR"sv,
};

int main(int argc, const char *argv[])
{
    using namespace vb;
    auto all_arguments = maker::build_arguments(argc, argv);

    auto args = maker::drop_command(all_arguments);

    auto main_options = maker::Stage::main_arguments(args);

    if (auto found = maker::find_argument(main_options, "-h"sv, "--help"sv, "-?"sv); found.has_value()) {
        std::println("Usage: {} «Options» ---«stage» options for stages", maker::command_from_args(all_arguments));
        for(auto stage : vb::maker::all_stages) {
            std::println("\t---{} : stage {} - {}",stage.option(), stage.name(), stage.information()); 
        }
        std::println("\t--help, -h, -? : {}", "This message");

        return 0;
    }

    auto target = std::string_view{};

    auto env = vb::env::environment{};

    for (auto var_name : default_environment) {
        env.import(var_name);
    }

    if (!main_options.empty()) {
        target = main_options.front();
    }

    auto root = std::filesystem::current_path();
    root = maker::builders::git_root_locator(root).value_or(root);

    auto builder = maker::builder{};
    auto current = maker::work_dir{root};
    for (auto stage : maker::all_stages) {
        builder = maker::builders::select(current, stage, env);
        if (builder && builder.required()) {
            break;
        }
    }

    if (!builder) {
        std::println(std::cerr, "Could not find an applicable builded for `{}`", current.path().string());
        return 1;
    }

    while (builder) {
        std::println("Running stage {} → {}:", builder.stage(), builder);

        auto arguments = maker::argument_list(builder.stage().filter_arguments(all_arguments));
        auto result = builder.run(target, arguments);

        std::println("{}", result);

        if (!result) {
            std::println("🚫 Builder {} failled at stage {} \n{}", builder.name(), builder.stage(), result);
            return 1;
        }

        builder = builder.next_builder();
    }
    return 0;
}
