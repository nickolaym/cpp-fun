#include "compose.h"
#include <gtest/gtest.h>

TEST(alternatives, no_funs) {
    constexpr auto f = carry_simple_alternatives();

    int x = 123;

    static_assert(std::is_same_v< decltype(f(+x)), int&& >);
    static_assert(std::is_same_v< decltype(f(x)), int& >);
    static_assert(std::is_same_v< decltype(f(std::as_const(x))), const int& >);
    static_assert(std::is_same_v< decltype(f(std::move(x))), int&& >);

    EXPECT_EQ(&f(x), &x);
    auto&& fx = f(std::move(x));
    EXPECT_EQ(&fx, &x);
}

TEST(alternatives, no_appropriate_fun) {
    constexpr auto f = carry_simple_alternatives([]{}, [](int, int){}, [](int*){});

    int x = 123;

    static_assert(std::is_same_v< decltype(f(+x)), int&& >);
    static_assert(std::is_same_v< decltype(f(x)), int& >);
    static_assert(std::is_same_v< decltype(f(std::as_const(x))), const int& >);
    static_assert(std::is_same_v< decltype(f(std::move(x))), int&& >);

    EXPECT_EQ(&f(x), &x);
    auto&& fx = f(std::move(x));
    EXPECT_EQ(&fx, &x);
}

TEST(alternatives, appropriate_fun) {
    constexpr auto f = carry_simple_alternatives(
        []{}, [](int, int){}, [](int*){}, // ignored
        [](int x) { return x + 0.; },
        [](int x) { return 'a'; } // unreachable
    );

    int x = 123;

    static_assert(std::is_same_v< decltype(f(+x)), double >);
    static_assert(std::is_same_v< decltype(f(x)), double >);
    static_assert(std::is_same_v< decltype(f(std::as_const(x))), double >);

    EXPECT_EQ(f(x), x + 0.);
}

TEST(alternatives, appropriate_fun_rvref) {
    double z = 777;
    const auto f = carry_simple_alternatives(
        []{}, [](int, int){}, [](int*){}, // ignored
        [&z](int x) -> double&& { return std::move(z); },
        [](int x) { return 'a'; } // unreachable
    );

    int x = 123;

    static_assert(std::is_same_v< decltype(f(+x)), double&& >);
    static_assert(std::is_same_v< decltype(f(x)), double&& >);
    static_assert(std::is_same_v< decltype(f(std::as_const(x))), double&& >);

    auto&& fx = f(x);
    EXPECT_EQ(&fx, &z);
}

TEST(alternatives, appropriate_fun_void) {
    bool touch = false;
    const auto f = carry_simple_alternatives(
        []{}, [](int, int){}, [](int*){}, // ignored
        [&](int x) { touch = true; },
        [](int x) { return 'a'; } // unreachable
    );

    int x = 123;

    static_assert(std::is_same_v< decltype(f(+x)), void >);
    static_assert(std::is_same_v< decltype(f(x)), void >);
    static_assert(std::is_same_v< decltype(f(std::as_const(x))), void >);

    f(x);
    EXPECT_TRUE(touch);
}

TEST(alternatives, appropriate_fun_distinct_inputs) {
    const auto f = carry_simple_alternatives(
        [](int&&) -> int* { return nullptr; },
        [](int&) -> int** { return nullptr; },
        [](int const&) -> int*** { return nullptr; }
    );

    int x = 123;

    static_assert(std::is_same_v< decltype(f(+x)), int* >);
    static_assert(std::is_same_v< decltype(f(x)), int** >);
    static_assert(std::is_same_v< decltype(f(std::as_const(x))), int*** >);
}
