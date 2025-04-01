#include "builders.hpp"
#include <ostream>

using namespace vb;


int main()
{
    maker::work_dir current{};
    maker::builder builder = maker::builders::select(current);

    if (!builder) {
        std::println(std::cerr, "Could not find an applicable builded for `{}`", current.path().string()); 
        return 1;
    }

    while (builder) {
        builder.run();
        builder = builder.next();
    }
    return 0;
}
