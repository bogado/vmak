#include "stage.hpp"

#include <filesystem>
#include <concepts>
#include <optional>

namespace vb::mak {

namespace prj {

template <typename PROJECT_T>
concept is_project = std::is_default_constructible_v<PROJECT_T> 
    && std::constructible_from<std::filesystem::path>
    && requires (const PROJECT_T prj, std::filesystem::path root)
{
    { prj.root() } ->  std::same_as<std::filesystem::path>;
    { prj.file(std::string{}) } -> std::same_as<std::optional<std::filesystem::path>>;
};

struct basic_project
{
    using path = std::filesystem::path;

    path root_folder;

    basic_project(path r) :
        root_folder{r}
    {}

    path root() const noexcept
    {
        return root_folder;
    }

    std::optional<path> file(std::string name) const noexcept
    {
        auto file = (root_folder / name);
        if (!std::filesystem::is_regular_file(file))
        {
            return std::nullopt;
        }
        return file;
    }
};

}
}
