#include <type_traits>
#include <util/execution.hpp>
#include <util/filesystem.hpp>

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <ranges>
#include <utility>

namespace vb::mak {

struct stage_type {
    enum class state : std::int8_t {
        PENDING = -1,
        INVALID = 0,
        STARTED,
        DONE,
        BLOCKED,
        NOT_REQUIRED,
        FAILED
    };

    using substages_type = std::span<std::unique_ptr<stage_type>>;

    stage_type() = default;
    stage_type(const stage_type&) = default;
    stage_type(stage_type&&) = default;
    stage_type& operator=(const stage_type&) = default;
    stage_type& operator=(stage_type&&) = default;

    virtual ~stage_type() = default;
    virtual fs::path root() const = 0;
    virtual fs::path work_dir() const = 0;
    virtual state status() const = 0;
    virtual void run() = 0;
};

template <std::constructible_from<fs::path> IMPL>
auto as_stage(IMPL&& impl) {
    struct stage_impl final : stage_type {
        IMPL impl;

        stage_impl(IMPL&& root) :
            impl{std::move(root)}
        {}
        
        fs::path root() const override {
            return impl.root();
        }

        fs::path work_dir() const override {
            return impl.work_dir();
        }

        stage_type::state status() const override {
            return impl.status();
        }

        void run() override {
            impl.run();
        }
    };

    return std::make_unique<stage_impl>(std::forward<IMPL>(impl));
}

template <typename STAGE_T>
concept is_build_stage = requires(STAGE_T stage, STAGE_T mutable_stage) {
    requires std::constructible_from<STAGE_T, fs::path>; // This is the root of the project.
    { *as_stage(stage) } -> std::derived_from<stage_type>;
    { STAGE_T::is_project(fs::path{}) } -> std::convertible_to<bool>;
};

template <static_string COMMAND, static_string CONFIG_FILE>
struct basic_stage {
    using state = stage_type::state;
    using enum state;

    stage_type::state current_status = PENDING;
    fs::path prj_root;

    explicit basic_stage(fs::path _root) :
        prj_root{_root}
    {}

    auto root() const {
        return prj_root;
    }

    auto work_dir() const {
        return root();
    }

    auto status() const {
        return current_status;
    }

    void run() {
        current_status = STARTED;
        auto run = execution{io_set::NONE};
        run.execute(std::string{COMMAND.view()}, work_dir());
        auto result = run.wait();
        current_status = result == 0 ? DONE : FAILED;
    }

    static auto is_project(fs::path root) {
        static const auto config = fs::path{CONFIG_FILE.view()};
        return fs::is_regular_file(root / config);
    }
};

using make_stage = basic_stage<"make", "makefile">;
using ninja_stage = basic_stage<"ninja", "build.ninja">;

static_assert(as_stage(make_stage{fs::path{"/"}})->status() == stage_type::state::PENDING);
static_assert(is_build_stage<make_stage>);
}
