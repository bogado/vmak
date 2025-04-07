#ifndef INCLUDED_RESULT_HPP
#define INCLUDED_RESULT_HPP

#include "util/execution.hpp"

#include <format>
#include <vector>
#include <iterator>

namespace vb {

struct execution_result {
    enum class type {
        NOT_DONE,
        NOT_NEEDED,
        SUCCESS,
        SOFT_FAILURE,
        FAILURE,
        PANIC
    };
    using enum type;

    std::vector<std::string> error_output;
    int exit_code = 0;
    type status = SUCCESS;

    constexpr operator bool() const {
        switch (status) {
        case NOT_NEEDED:
        case SOFT_FAILURE:
        case SUCCESS:
            return true;
        default:
            return false;
        }
    }

    execution_result(type st = NOT_DONE) :
        status{st}
    {}

    execution_result(execution& exec)
        : error_output{std::ranges::to<std::vector>(exec.lines<std_io::ERR>())}, exit_code{exec.wait()},
          status{exit_code != 0         ? FAILURE
                 : error_output.empty() ? SUCCESS
                                        : SOFT_FAILURE} {}

    execution_result merge(execution_result other) const
    {
        if (status > other.status) {
            other.status = status;
        }
        std::ranges::copy(error_output, std::back_inserter(other.error_output));
        return other;
    }
};

}

template<>
struct std::formatter<vb::execution_result, char>
{
    static constexpr auto length = 80;
    std::string divider;

    template <class PARSE_CONTEXT>
    constexpr PARSE_CONTEXT::iterator parse(PARSE_CONTEXT& context)
    {
        divider = std::string(length, '-');
        divider.push_back('\n');
        return context.begin();
    }

    template <class FORMAT_CONTEXT>
    constexpr FORMAT_CONTEXT::iterator format(const vb::execution_result& result, FORMAT_CONTEXT& context) const
    {
        using enum vb::execution_result::type;
        std::string status;
        switch (result.status) {
        case SUCCESS:
            status = "Success";
            break;
        case SOFT_FAILURE:
            status = "Success with messages";
            break;
        case FAILURE: 
            status = "Failure";
            break;
        case NOT_DONE:
        case NOT_NEEDED:
            status = "Skipped";
            break;
        case PANIC:
            status = "Panic";
            break;
        }

        auto ext = std::format(" → {}\n", result.exit_code);
        auto out = std::ranges::copy(status, context.out()).out;
        out      = std::ranges::copy(ext, out).out;

        if (!result.error_output.empty()) {
            out = std::ranges::copy(divider, out).out;
            for (auto line: result.error_output) {
                out = std::ranges::copy(line, out).out;
            }
            out = std::ranges::copy(divider, out).out;
        }
        return out;
    }
};
#endif // INCLUDED_RESULT_HPP
