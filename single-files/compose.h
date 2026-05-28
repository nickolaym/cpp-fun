#pragma once

#include <type_traits>
#include <utility>
#include <concepts>

#include "callable_concept.h"

// спасибо rg45 @ rsdn.ru

#define FWD(a) std::forward<decltype(a)>(a)

#define FFWD(f, a) f(FWD(a))

#define RETURN_IF_RESOLVED(e) requires requires { e; } { return (e); }

#define CRITERIA template<class>class

// дефолтный критерий остановки "подходит всё" (для альтернатив)
template<class>using AcceptAll = std::true_type;
// дефолтный критерий остановки "не подходит ничего" (для цепочки)
template<class>using RejectAll = std::false_type;

// фабрика критериев
template<class Dst> struct accepts {
    template<class T> using criteria = std::is_same<T, Dst>;
};

// концепт, проверяющий критерий
template<class T, CRITERIA C>
concept fits_criteria = C<T>::value;

template<class T, CRITERIA C>
concept not_fits_criteria = !C<T>::value;

// композиция "перебор альтернатив до первой подходящей функции"
// (которую можно вызвать с данным аргументом и чей результат подходит под критерий остановки)

namespace alt_helpers {

template<class F, CRITERIA C, class A>
concept appropriate_alternative = Callable<F> && fits_criteria<std::invoke_result_t<F, A>, C>;
// 1) можно вызвать
// 2) и результат подходит под критерий остановки

template<class F, CRITERIA C, class A>
concept inappropriate_alternative = Callable<F> && !appropriate_alternative<F, C, A>;
// или нельзя вызвать, или результат неподходящий


#if 0
template<class...> struct typelist {};

// resolver<...>{}(arg, f1, f2, ..., fk, ..., fn)
// вызывает первую подходящую функцию из f1...fn (скажем, fk)
//
// если подходящих нет, то не резолвится (нет подходящей сигнатуры)
// и это можно диагностировать через !requires
// или через std::bool_constant, от которого он унаследован.

template<CRITERIA C, class A, class RejectedTL, Callable... Fs>
struct resolver;

template<CRITERIA C, class A, Callable... Rs>
struct resolver<C, A, typelist<Rs...>> // нет больше функций
: std::false_type
{
    // constexpr A operator()(A a, Rs...) const { return a; }
};

template<CRITERIA C, class A, Callable... Rs, appropriate_alternative<C, A> F, Callable... Gs>
struct resolver<C, A, typelist<Rs...>, F, Gs...> // F(A) fits to C
: std::true_type
{
    constexpr decltype(auto) operator()(A a, Rs..., F f, Gs...) const {
        return FFWD(f, a);
    }
};

template<CRITERIA C, class A, Callable... Rs, inappropriate_alternative<C, A> F, Callable... Gs>
struct resolver<C, A, typelist<Rs...>, F, Gs...> // F(A) fits to C
: resolver<C, A, typelist<Rs..., F>, Gs...> {};

#endif

template<class F> struct funref {
    F f;
};

template<Callable F> struct solution {
    static constexpr bool value = true;
    F f; // ideal reference

    template<class G>
    constexpr decltype(auto) operator || (funref<G>) const { return *this; }

    constexpr decltype(auto) operator()(auto&& a) const {
        return FFWD(f, a);
    }
};

template<CRITERIA C, class A> struct seed {
    static constexpr bool value = false;

    template<class F>
    constexpr decltype(auto) operator || (const funref<F>& ff) const {
        if constexpr (appropriate_alternative<F, C, A>)
            return solution<F>{FWD(ff.f)};
        else
            return *this;
    }
};

template<CRITERIA C> struct run_alternatives {
    // constexpr auto get_resolver(auto&& a, Callable auto&&... fs) const {
    //     return resolver<C, decltype(a), typelist<>, decltype(fs)...>{};
    // }

    constexpr auto get_alternative(auto&& a, Callable auto&&... fs) const {
        return ( seed<C, decltype(a)>{} || ... || funref<decltype(fs)>{FWD(fs)} );
    }

    constexpr bool resolved(auto&& a, Callable auto&&... fs) const {
        // return get_resolver(FWD(a), FWD(fs)...).value;
        return get_alternative(FWD(a), FWD(fs)...).value;
    }

    constexpr decltype(auto) operator()(auto&& a, Callable auto&&... fs) const
    RETURN_IF_RESOLVED(
        // get_resolver(FWD(a), FWD(fs)...)(FWD(a), FWD(fs)...)
        get_alternative(FWD(a), FWD(fs)...)(FWD(a))
    )
};

} // namespace alt_helpers




constexpr auto fallback_alternative = [](auto&& a) -> decltype(auto) { return FWD(a); };

template<CRITERIA C> constexpr alt_helpers::run_alternatives<C> run_alternatives;

template<CRITERIA C> constexpr auto carry_alternatives = [](Callable auto&&... fs) {
    return [... fs = FWD(fs)](auto&& a)
    -> decltype(auto)
    RETURN_IF_RESOLVED( (run_alternatives<C>(FWD(a), fs...)) );
};

constexpr auto run_simple_alternatives = run_alternatives<AcceptAll>;
constexpr auto carry_simple_alternatives = carry_alternatives<AcceptAll>;

////////////////////////////////////////////////////////////////////////////////

// выполнение цепочки

namespace seq_helpers {

// если аргумент удовлетворяет критерию остановки, то останавливаемся
template<class T, CRITERIA C>
concept stopping_arg = fits_criteria<std::decay_t<T>, C>;

// если не удовлетворяет - пытаемся продолжить
template<class T, CRITERIA C>
concept running_arg = not_fits_criteria<std::decay_t<T>, C>;

// неподходящий шаг
template<class F, class A>
concept inappropriate_step = Callable<F> && !std::invocable<F, A>;

// подходящий шаг
template<class F, class A>
concept appropriate_step = Callable<F> && std::invocable<F, A>;

// остановка сразу после шага
template<class F, CRITERIA C, class A>
concept stopping_step = stopping_arg<std::invoke_result_t<F, A>, C>;

// первый шаг подходит и проходит дальше
template<class F, CRITERIA C, class A>
concept running_step = running_arg<std::invoke_result_t<F, A>, C>;

template<CRITERIA C>
struct run_sequence {
    decltype(auto) operator()(auto&& arg, Callable auto&&... fs) const {
        return run<decltype(arg)>(FWD(arg), FWD(fs)...);
    }

    // тип Arg может быть ссылкой или значением
    // и выводится
    // - при запуске - из универсальной ссылки на аргумент
    // - при продолжении - из результата функции

    // остановка на аргументе
    template<class Arg>
    Arg run(stopping_arg<C> auto&& arg, Callable auto&&... fs) const
    { return FWD(arg); }

    template<class Arg>
    Arg run(running_arg<C> auto&& arg) const
    { return FWD(arg); }

    template<class Arg>
    Arg run(
        running_arg<C> auto&& arg,
        inappropriate_step<decltype(arg)> auto&& f,
        auto&&... fs
    ) const
    { return FWD(arg); }

    // остановка на первой функции
    template<class>
    decltype(auto) run(
        running_arg<C> auto&& arg,
        stopping_step<C, decltype(arg)> auto&& f,
        auto&&... fs
    ) const
    { return FFWD(f, arg); }

    template<class>
    decltype(auto) run(
        running_arg<C> auto&& arg,
        running_step<C, decltype(arg)> auto&& f
    ) const
    { return FFWD(f, arg); }

    template<class>
    decltype(auto) run(
        running_arg<C> auto&& arg,
        running_step<C, decltype(arg)> auto&& f,
        inappropriate_step<decltype(FFWD(f, arg))> auto& g,
        auto&&... fs
    ) const
    { return FFWD(f, arg); }

    // продолжаем по цепочке
    template<class>
    decltype(auto) run(
        running_arg<C> auto&& arg,
        running_step<C, decltype(arg)> auto&& f,
        appropriate_step<decltype(FFWD(f, arg))> auto& g,
        auto&&... fs
    ) const
    { return run<decltype(FFWD(f, arg))>(FFWD(f, arg), FWD(g), FWD(fs)...); }
};

} // namespace seq_helpers

template<CRITERIA C> constexpr auto run_sequence = seq_helpers::run_sequence<C>{};

template<CRITERIA C> constexpr auto carry_sequence = [](Callable auto&&... fs) {
    return [... fs = FWD(fs)](auto&& a) -> decltype(auto) {
        return run_sequence<C>(FWD(a), fs...);
    };
};

constexpr auto run_simple_sequence = run_sequence<RejectAll>;
constexpr auto carry_simple_sequence = carry_sequence<RejectAll>;
