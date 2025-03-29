#include "util/filesystem.hpp"
#include "util/environment.hpp"
#include <concepts>
#include <filesystem>
#include <ranges>
#include <util/execution.hpp>
#include <vector>

namespace vb::maker {

using namespace std::literals;

using arguments = std::vector<std::string_view>;

template <typename WORKDIR>
concept is_work_directory = requires(const WORKDIR dir) {
    { dir.path() } -> std::same_as<fs::path>;
    { dir.has(std::string{"filename"}) } -> std::same_as<bool>;
    { dir.execute("command"sv, arguments{}) } -> std::convertible_to<bool>;
    { dir.execute("command"sv, env::environment{}) } -> std::convertible_to<bool>;
    { dir.execute("command"sv, arguments{}, env::environment{}) } -> std::convertible_to<bool>;
};

struct work_dir {
    fs::path root;
    
    fs::path path() const { 
        return root;
    }

    bool has(std::string_view file) const {
        return fs::exists(root / file);
    }

    bool execute(std::string_view command, std::ranges::contiguous_range auto args, env::environment env = {})
    {
        execution executer;
        return executer.execute(fs::path{command}, args, env, root) != 0;
    }
};


}

