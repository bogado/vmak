#include <filesystem>
#include <concepts>
#include <utility>
#include <array>
#include <ranges>
#include <algorithm>

namespace vb::mak {

namespace prj {

enum class stage : signed short {
    FAILED = -1,
    CONFIG,
    BUILD,
    INVALID,
    DONE = 128,
};

enum class status : signed short {
    FAILED = -1,
    PENDING,
    STARTED,
    DONE = 128,
};

template <typename ENUM_T>
requires std::is_enum_v<ENUM_T>
constexpr inline auto operator <=>(const stage& a, const stage& b) {
    return static_cast<std::underlying_type_t<ENUM_T>>(a) <=> static_cast<std::underlying_type_t<ENUM_T>>(b);
};

static_assert(stage::CONFIG < stage::DONE);
static_assert(status::DONE > status::FAILED);

constexpr auto STAGES = static_cast<unsigned>(stage::INVALID);

struct state {
    std::array<status, STAGES> stages;

    constexpr status operator[](stage st) const {
        return stages[static_cast<unsigned short>(st)];
    }

    constexpr stage next() const {
        auto r = std::ranges::find_if(stages, [](const auto& s) -> bool {
            return s > status::DONE;
        });

        if (r != stages.end()) {
            return *r == status::DONE
                ? stage::DONE
                : stage::FAILED;
        } else {
            return static_cast<stage>(std::distance(stages.begin(), r));
        }
    }
};
    
template <typename PROJECT_T>
concept is_project = std::is_default_constructible_v<PROJECT_T> 
    && std::constructible_from<std::filesystem::path>
    && requires (const PROJECT_T prj, std::filesystem::path root)
{
    { prj.root() } ->  std::same_as<std::filesystem::path>;
    { prj.operator() } -> std::convertible_to<bool>;
    { prj.status() } -> std::same_as<status>;
};

struct basic_project
{
};

}
}
