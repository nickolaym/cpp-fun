#pragma once

#include <type_traits>
#include <utility>
#include <concepts>

// спасибо rg45 @ rsdn.ru

#define FWD(a) std::forward<decltype(a)>(a)

#define FFWD(f, a) f(FWD(a))

#define CRITERIA template<class>class

template<class T, CRITERIA C>
concept fits_criteria = C<T>::value;

template<class F, CRITERIA C, class A>
concept appropriate_function = fits_criteria<std::invoke_result_t<F, A>, C>;
// 1) can invoke - invoke_result_t is instantiated
// 2) the result fits given criteria

template<class F, CRITERIA C, class A>
concept inappropriate_function = !appropriate_function<F, C, A>;


template<class...> struct typelist {};

// композиция "перебор альтернатив до первой подходящей функции"

namespace alt_helpers {

// resolver<...>{}(arg, f1, f2, ..., fk, ..., fn)
// вызывает первую подходящую функцию из f1...fn (скажем, fk)
//
// если подходящих нет, то дефолтится к arg по идеальной ссылке
// TODO: не дефолтиться, а не резолвиться вовсе
// (а дефолт можно реализовать, добавив последнюю всеядную функцию-форвард)

template<CRITERIA C, class A, class RejectedTL, class... Fs>
struct resolver;

template<CRITERIA C, class A, class... Rs>
struct resolver<C, A, typelist<Rs...>> // нет больше функций
{
    constexpr A operator()(A a, Rs...) const {
        return a;
    }
};

template<CRITERIA C, class A, class... Rs, appropriate_function<C, A> F, class... Gs>
struct resolver<C, A, typelist<Rs...>, F, Gs...> // F(A) fits to C
{
    constexpr decltype(auto) operator()(A a, Rs..., F f, Gs...) const {
        return FFWD(f, a);
    }
};

template<CRITERIA C, class A, class... Rs, inappropriate_function<C, A> F, class... Gs>
struct resolver<C, A, typelist<Rs...>, F, Gs...> // F(A) fits to C
: resolver<C, A, typelist<Rs..., F>, Gs...> {};

template<CRITERIA C> struct run_alternatives {
    constexpr decltype(auto) operator()(auto&& a, auto&&... fs) const {
        return resolver<C, decltype(a), typelist<>, decltype(fs)...>{}(FWD(a), FWD(fs)...);
    }
};

} // namespace alt_helpers

template<CRITERIA C> constexpr alt_helpers::run_alternatives<C> run_alternatives;

template<CRITERIA C> constexpr auto carry_alternatives = [](auto... fs) {
    return [fs...](auto&& a) -> decltype(auto) { return run_alternatives<C>(FWD(a), fs...); };
};

template<class>using AcceptAll = std::true_type;

constexpr auto run_simple_alternatives = run_alternatives<AcceptAll>;
constexpr auto carry_simple_alternatives = carry_alternatives<AcceptAll>;