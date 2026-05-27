#include "callable_concept.h"


struct F {
    void operator()() const {}
};
struct G {
    void operator()(char) const {}
    void operator()(int) const {}
};
struct H {};

static_assert(Callable<F>);
static_assert(Callable<G>);
static_assert(!Callable<H>);

// corner case: operator() exists but unavailable
struct D {
    void operator()() const = delete;
};
class E {
    void operator()() const {}
};

static_assert(Callable<D>);
static_assert(Callable<E>);
static_assert(![](auto...){ return requires { D{}(); }; }());
static_assert(![](auto...){ return requires { E{}(); }; }());

static_assert(Callable<decltype([]{})>);
static_assert(!Callable<decltype(123)>);

static auto fun = [](auto) {};
static auto& fun_ref = fun;
static_assert(Callable<decltype(fun)>);
static_assert(Callable<decltype(fun_ref)>);
static_assert(Callable<decltype(std::as_const(fun_ref))>);
static_assert(Callable<decltype(std::move(fun_ref))>);