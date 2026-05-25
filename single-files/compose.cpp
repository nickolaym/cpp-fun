#include "compose.h"
#include <gtest/gtest.h>

TEST(alternatives, no_fallback) {
    constexpr auto f = carry_simple_alternatives();

    int x = 123;

    constexpr auto res = alt_helpers::run_alternatives<AcceptAll>{}.get_resolver(x);
    static_assert(!res.value);

    // requires failure is not an error in template context only!
    // (must depend on template params)
    [&](auto y){
        static_assert(!requires{res(y);});
        static_assert(!requires{f(y);});
    }(x);
}

TEST(alternatives, no_funs) {
    constexpr auto f = carry_simple_alternatives(fallback_alternative);

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
    constexpr auto f = carry_simple_alternatives(
        []{}, [](int, int){}, [](int*){},
        fallback_alternative
    );

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

template<int n> using int_tag = std::integral_constant<int, n>;

template<int n> auto make_echo() { return [](int_tag<n>) { return int_tag<-n>{}; }; }

TEST(alternatives, association) {
    const auto f = carry_simple_alternatives(
        make_echo<1>(),
        make_echo<2>(),
        carry_simple_alternatives(
            make_echo<3>(),
            make_echo<4>()
        ),
        make_echo<5>()
    );

    static_assert(f(int_tag<1>{}) == int_tag<-1>{});
    static_assert(f(int_tag<2>{}) == int_tag<-2>{});
    static_assert(f(int_tag<3>{}) == int_tag<-3>{});
    static_assert(f(int_tag<4>{}) == int_tag<-4>{});
    static_assert(f(int_tag<5>{}) == int_tag<-5>{});

    [&](auto t){
        static_assert(!requires{ f(t); });
    }(int_tag<0>{});
}