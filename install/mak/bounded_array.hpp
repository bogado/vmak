#ifndef INCLUDE_MAK_BOUNDED_ARAY_HPP_
#define INCLUDE_MAK_BOUNDED_ARAY_HPP_

#include <cstddef>
#include <iterator>
#include <string_view>

namespace vb {

template <typename T, T BOUND = T{}>
struct bounded_array_view {
    static constexpr auto bound = BOUND;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = bounded_array_view;
    using sentinel = std::default_sentinel_t;

    pointer head;

    constexpr auto begin() const {
        return *this;
    }

    constexpr auto end() const {
        return std::default_sentinel;
    }

    constexpr auto size() const {
        return std::ranges::distance(*this, end());
    }

    constexpr const value_type& operator*() const {
        return *head;
    }

    constexpr bool operator==(const bounded_array_view&) const = default;
    constexpr bool operator!=(const bounded_array_view&) const = default;

    constexpr bool operator==(const std::default_sentinel_t&) const 
    {
        return head == nullptr || *head == bound;
    }

    constexpr bool operator!=(const std::default_sentinel_t&) const
    {
        return head != nullptr && *head != bound;
    }

    friend constexpr bool operator==(std::default_sentinel_t, const bounded_array_view& self)
    {
        return self != std::default_sentinel;
    }

    constexpr auto& operator++()
    {
        if (*this != std::default_sentinel) {
            head++;
        }
        return *this;
    }

    constexpr auto operator++(int)
    {
        if (*this != std::default_sentinel) {
            return *this;
        }
        auto result = *this;
        ++(*this);
        return result;
    }
};

namespace static_test {

constexpr auto t = bounded_array_view{"test\0"};
static_assert(*t == 't');
static_assert(std::string_view(t.head) == "test");
static_assert(std::string_view(t.begin().head) == "test");
static_assert(std::string_view((++t.begin()).head) == "est");
static_assert(t.size() == 4);
}

}

#endif  // INCLUDE_MAK_BOUNDED_ARAY_HPP_
