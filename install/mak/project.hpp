#include <filesystem>
#include <concepts>
#include <utility>
#include <array>

namespace vb::mak {

namespace prj {

enum class stage : unsigned short {
    CONFIG,
    BUILD,
    DONE
};

enum class status : unsigned short {
    PENDING,
    STARTED,
    DONE,
    FAILED
};

template <typename ENUM_TYPE>
concept enumeration = std::is_enumeration_v<ENUM_TYPE>;

template <enumeration ENUM_TYPE>
auto to_underling(enumeration en) {
    return static_cast<std::to_underling_t<ENUM_TYPE>>(en);
}

constexpr auto STAGES = to_underling(stage::DONE);

struct state : std::array<status, STAGES> {
    using storage = std::array<status, STAGES>;

    state()
        : storage{}
    {
        std::ranges::fill(*this, PENDING);
    }

    status& operator[](stage st) const {
        return (*this)[static_cast<unsigned short>(st)];
    }

    
};

template <status::state >
constexpr bool needs)

namespace static_test {
    static constexpr auto t = status::BUILD_DONE bitand status::CONFIG_STEP bitand status::BUILD_STEP;
    static_assert(!needs<status::BUILD_STEP>(t));
}

template <typename PROJECT_T>
concept is_project = std::is_default_constructible_v<PROJECT_T> 
    && std::constructible_from<std::filesystem::path>
    && requires (const PROJECT_T prj, std::filesystem::path root)
{
    { prj.root() } ->  std::same_as<std::filesystem::path>;
    { prj.operator() } -> std::convertible_to<bool>;
    { prj.status() } -> std::same_as<status>
};

struct basic_project()
{
};

}
