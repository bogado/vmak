#include "builders.hpp"

#include <ostream>
#include <ranges>
#include <string_view>
#include <vector>

using namespace vb;

int
main(int argc, const char *argv[])
{
    auto args   = std::ranges::to<std::vector>(std::span(argv, static_cast<std::size_t>(argc)) | std::views::drop(1));
    auto target = std::string_view{};
    if (!args.empty()) {
        target = args.front();
    }

    maker::work_dir current{};
    maker::builder  builder = maker::builders::select(current);

    if (!builder) {
        std::println(std::cerr, "Could not find an applicable builded for `{}`", current.path().string());
        return 1;
    }

    while (builder) {
        std::print("Running {}:", builder);

        auto result = builder.execute(target);

        std::println("{}", result);

        if (!result) {
            return 1;
        }
        builder = builder.next();
    }
    return 0;
}
