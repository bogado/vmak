#include <util/execution.hpp>
#include <util/filesystem.hpp>
#include <util/string.hpp>

#include <filesystem>
#include <string>
#include <system_error>
#include <type_traits>
#include <concepts>

namespace vb::mak {

struct no_action{};

template <typename ACTION_T>
concept is_action = (
        std::is_invocable_v<ACTION_T>
        && std::is_same_v<std::invoke_result_t<ACTION_T>, std::error_code>
    ) || std::is_class_v<no_action>;

struct stage_base {
    enum class state {
        CREATED,
        STARTED,
        DONE,
        ERROR
    };

    state current_state = state::CREATED;

    stage_base() = default;
    stage_base(const stage_base&) = default;
    stage_base(stage_base&&) = default;
    stage_base& operator=(const stage_base&) = default;
    stage_base& operator=(stage_base&&) = default;

    virtual ~stage_base() = default;
    virtual fs::path root() const = 0;
    virtual fs::path work_dir() const = 0;

    state status() const
    {
        return current_state;
    }

    state& status()
    {
        return current_state;
    }
};

template <typename STAGE_T>
concept is_build_stage = std::is_constructible_v<STAGE_T, fs::path> // This is the root of the project.
    && std::is_base_of_v<stage_base, STAGE_T>
    && is_action<STAGE_T>
    && requires(STAGE_T stage) {
        { STAGE_T::is_project(fs::path{}) } -> std::convertible_to<bool>;
        { stage.next() } -> is_action;
};

bool has_next(is_build_stage auto& stage) {
    return std::same_as<std::invoke_result_t<decltype(stage.next())>, no_action>;
}

template <static_string COMMAND, static_string CONFIG_FILE, is_action NEXT_ACTION = no_action>
struct basic_stage : stage_base {
    fs::path prj_root;

    explicit basic_stage(fs::path _root) :
        prj_root{_root}
    {}

    fs::path root() const override
    {
        return prj_root;
    }

    fs::path work_dir() const override {
        return root();
    }

    std::error_code operator() () {
        status() = state::STARTED;
        auto run = execution{io_set::NONE};
        int result = 0;
        try {
            run.execute(std::string{COMMAND.view()}, work_dir());
            result = run.wait();
            status() = result == 0 ? state::DONE : state::ERROR;
        } catch (const std::system_error& error) {
            status() = state::ERROR;
            return error.code();
        }
        if (result != 0) {
            return std::error_code(1, std::generic_category());
        }
        return std::error_code{};
    }

    static auto is_project(fs::path root) {
        static const auto config = fs::path{CONFIG_FILE.view()};
        return fs::is_regular_file(root / config);
    }

    no_action next();
};

using make_stage = basic_stage<"make", "makefile">;
using ninja_stage = basic_stage<"ninja", "build.ninja">;
using jekyll_stage = basic_stage<"jekyll", "_config.yml">;
using rust_stage = basic_stage<"cargo build", "Cargo.toml">;
using gradle_stage = basic_stage<"./gradlew", "gradlew">;

static_assert(is_build_stage<make_stage>);
}
