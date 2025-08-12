#include "builders.hpp"
#include <util/converters.hpp>
#include <util/environment.hpp>
#include <util/options.hpp>

#include <print>
#include <ranges>
#include <string_view>
#include <vector>

using namespace vb;
constexpr auto default_environment = std::array{
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
    auto args = std::ranges::to<std::vector>(std::span(argv, static_cast<std::size_t>(argc)) | std::views::drop(1));

    auto target = std::string_view{};

    auto env = vb::env::environment{};

    for (auto var_name : default_environment) {
        env.import(var_name);
    }

    if (!args.empty()) {
        target = args.front();
    }

    auto builder = maker::builder{};
    auto current = maker::work_dir{};
    for (auto task : vb::maker::all_tasks) {
        builder = maker::builders::select(current, task, env);
        if (builder && builder.required()) {
            break;
        }
    }

    if (!builder) {
        std::println(std::cerr, "Could not find an applicable builded for `{}`", current.path().string());
        return 1;
    }

    while (builder) {
        std::println("Running {}:", builder);

        auto result = builder.run(target);

        std::println("{}", result);

        if (!result) {
            std::println("🚫 Builder {} failled \n{}", builder.name(), builder);
            return 1;
        }

        builder = builder.next_builder();
    }
    return 0;
}
