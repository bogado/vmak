#include "util/execution.hpp"
#include "util/string.hpp"
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <util/filesystem.hpp>
#include <ranges>

namespace vb::mak {

struct stage {
    enum class state : std::int8_t {
        PENDING = -1,
        INVALID = 0,
        STARTED,
        DONE,
        BLOCKED,
        NOT_REQUIRED,
        FAILED
    };

    virtual ~stage() = default;
    virtual fs::path root() const = delete;
    virtual fs::path work_dir() const = delete;
    virtual state status() const = delete;
    virtual void run() = delete;
};

template <typename STAGE_T>
concept is_stage = requires(const STAGE_T stage, STAGE_T mutable_stage) {
    requires std::constructible_from<STAGE_T, fs::path>; // This is the root of the project.
    { stage.root() } -> std::same_as<fs::path>;
    { stage.work_dir() } -> std::same_as<fs::path>;
    { stage.status() } -> std::same_as<stage::state>;
    { mutable_stage.run() };
};

template <is_stage IMPL>
auto make_stage(IMPL&& impl) {
    struct stage_impl final : stage {
        IMPL impl;

        stage_impl(fs::path root) :
            impl{root}
        {}

        fs::path work_dir() const override {
            return impl.work_dir();
        }

        stage::state status() const override {  
            return impl.status();
        }

        void run() override {
            impl.run();
        }
    };
    return stage_impl{std::move(impl)};
}

template <static_string COMMAND>
struct basic_stage {
    using state = stage::state;
    using enum state;

    stage::state current_status = PENDING;
    fs::path prj_root;

    basic_stage(fs::path _root) :
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
        run.execute(std::string{COMMAND}, work_dir());
        auto result = run.wait();
        current_status = result == 0 ? DONE : FAILED;
    }       
};

}
