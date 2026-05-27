#include "compose.h"
#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////
// Alternatives

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

////////////////////////////////////////////////////////////////////////////////
// Sequence

template<class T> using IsInt = std::is_same<T, int>;

TEST(sequence, stopping_arg) {
    constexpr auto seq = run_sequence<IsInt>;

    static_assert(seq_helpers::stopping_arg<int, IsInt>);
    static_assert(seq_helpers::stopping_arg<int&&, IsInt>);
    static_assert(seq_helpers::stopping_arg<const int&, IsInt>);

    int x;

    // will not even try to call any function

    // no functions
    static_assert(std::is_same_v<decltype(seq(std::move(x))), int&&>);
    static_assert(std::is_same_v<decltype(seq(x)), int&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x))), const int&>);

    // unreachable functions
    constexpr auto f = [](auto...) { static_assert(false); return 'a'; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f)), int&&>);
    static_assert(std::is_same_v<decltype(seq(x, f)), int&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f)), const int&>);

    constexpr auto g = [](auto...) { static_assert(false); return 123.45; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f, g)), int&&>);
    static_assert(std::is_same_v<decltype(seq(x, f, g)), int&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f, g)), const int&>);
}

TEST(sequence, cannot_call_f) {
    constexpr auto seq = run_sequence<IsInt>;

    static_assert(seq_helpers::running_arg<char, IsInt>);
    static_assert(seq_helpers::running_arg<char&&, IsInt>);
    static_assert(seq_helpers::running_arg<const char&, IsInt>);

    char x;

    static_assert(std::is_same_v<decltype(seq(std::move(x))), char&&>);
    static_assert(std::is_same_v<decltype(seq(x)), char&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x))), const char&>);

    // stop before inappropriate function
    constexpr auto f = [](auto*) { static_assert(false); return 123; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f)), char&&>);
    static_assert(std::is_same_v<decltype(seq(x, f)), char&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f)), const char&>);

    // unreachable function
    constexpr auto g = [](auto...) { static_assert(false); return 123; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f, g)), char&&>);
    static_assert(std::is_same_v<decltype(seq(x, f, g)), char&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f, g)), const char&>);
}

TEST(sequence, stopping_f) {
    constexpr auto seq = run_sequence<IsInt>;
    char x;

    static_assert(std::is_same_v<decltype(seq(std::move(x))), char&&>);
    static_assert(std::is_same_v<decltype(seq(x)), char&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x))), const char&>);

    // stop with f(x)
    constexpr auto f = [](auto) { return 123; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f)), int>);
    static_assert(std::is_same_v<decltype(seq(x, f)), int>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f)), int>);

    // unreachable g
    constexpr auto g = [](auto...) { static_assert(false); return 123.45; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f, g)), int>);
    static_assert(std::is_same_v<decltype(seq(x, f, g)), int>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f, g)), int>);
}

TEST(sequence, f_returns_void) {
    constexpr auto seq = run_sequence<IsInt>;
    char x;

    // stop with f(x)
    constexpr auto f = [](auto) {};
    static_assert(std::is_same_v<decltype(seq(std::move(x), f)), void>);
    static_assert(std::is_same_v<decltype(seq(x, f)), void>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f)), void>);

    // inappropriate g: cannot call g(f(x)) because f(x) = void
    constexpr auto g = [](auto...) { static_assert(false); return 123; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f, g)), void>);
    static_assert(std::is_same_v<decltype(seq(x, f, g)), void>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f, g)), void>);
}

TEST(sequence, cannot_call_g) {
    constexpr auto seq = run_sequence<IsInt>;
    char x;

    // running f(x)
    constexpr auto f = [](auto) { return 123.4; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f)), double>);
    static_assert(std::is_same_v<decltype(seq(x, f)), double>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f)), double>);

    // cannot call g(f(x))
    constexpr auto g = [](auto*) { static_assert(false); return 123; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f, g)), double>);
    static_assert(std::is_same_v<decltype(seq(x, f, g)), double>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f, g)), double>);
}

TEST(sequence, cannot_call_g_return_rvalue_ref) {
    constexpr auto seq = run_sequence<IsInt>;
    char x;

    double z;
    // running f(x)
    const auto f = [&z](auto) -> double&& { return std::move(z); };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f)), double&&>);
    static_assert(std::is_same_v<decltype(seq(x, f)), double&&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f)), double&&>);

    // cannot call g(f(x))
    constexpr auto g = [](auto*) { static_assert(false); return 123; };
    static_assert(std::is_same_v<decltype(seq(std::move(x), f, g)), double&&>);
    static_assert(std::is_same_v<decltype(seq(x, f, g)), double&&>);
    static_assert(std::is_same_v<decltype(seq(std::as_const(x), f, g)), double&&>);

    auto&& y = seq(x, f, g);
    EXPECT_EQ(&y, &z);
}

TEST(sequence, chain_of_references) {
    constexpr auto seq = run_sequence<IsInt>;
    // running f(x)
    constexpr auto f = [](char&& t) -> char&& { t += 1; return std::move(t); };
    constexpr auto g = [](auto*) { static_assert(false); }; // inappropriate
    constexpr auto h = [](char t) -> int { return t+10; }; // stopping

    static_assert(std::is_same_v<decltype(seq('a', f, f, f)), char&&>);
    static_assert(std::is_same_v<decltype(seq('a', f, f, f, g)), char&&>);
    static_assert(std::is_same_v<decltype(seq('a', f, f, f, h, h)), int>);
    EXPECT_EQ(seq('a', f, f, f), 'd');
    EXPECT_EQ(seq('a', f, f, f, g), 'd');
    EXPECT_EQ(seq('a', f, f, f, h, h), 'd'+10);

    char a;

    a = 'a';
    auto&& fffx = seq(std::move(a), f, f, f);
    EXPECT_EQ(&fffx, &a);

    a = 'a';
    auto&& gfffx = seq(std::move(a), f, f, f, g);
    EXPECT_EQ(&gfffx, &a);
    EXPECT_EQ(a, 'd');

    a = 'a';
    auto&& hhfffx = seq(std::move(a), f, f, f, h, h);
    EXPECT_NE((void*)&hhfffx, (void*)&a);
    EXPECT_EQ(a, 'd');
    EXPECT_EQ(hhfffx, 'd'+10);
}

TEST(sequence, without_stop_criteria) {
    int count = 0;
    auto f = [&count](auto* p) { ++count; return *p; };
    auto fff = carry_simple_sequence(f, f, f);

    static_assert(Callable<decltype(f)>);

    static_assert(std::is_same_v<decltype(fff( (int)0 )), int&&>);
    static_assert(std::is_same_v<decltype(fff( (int*)nullptr )), int>);
    static_assert(std::is_same_v<decltype(fff( (int**)nullptr )), int>);
    static_assert(std::is_same_v<decltype(fff( (int***)nullptr )), int>);
    static_assert(std::is_same_v<decltype(fff( (int****)nullptr )), int*>);

    EXPECT_EQ((count = 0, fff((int)0), count), 0);
    EXPECT_EQ((count = 0, fff((int*)nullptr), count), 1);
    EXPECT_EQ((count = 0, fff((int**)nullptr), count), 2);
    EXPECT_EQ((count = 0, fff((int***)nullptr), count), 3);
    EXPECT_EQ((count = 0, fff((int****)nullptr), count), 3);
}
