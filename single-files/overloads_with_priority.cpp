#include <utility>

// макрос для идеальной передачи аргумента (объявленного как auto&&)
#define FWD(x) std::forward<decltype(x)>(x)

// суффикс фукнции или лямбды, работающий как SFINAE
#define TRY_RETURN(expr) -> decltype(expr) { return (expr); }

// монолитная лямбда (первоклассный объект - можно передавать как аргумент)
// представляющая семейство перегрузок одноимённых функций.
// здесь принципиально, что её семейство operator() повторяют SFINAE исходников
#define FUN_OBJECT(fun_family) \
    [](auto&&... xs) TRY_RETURN(fun_family(FWD(xs)...))

// лямбда - диспетчер семейства функций вида f(overloads::prio<N>, .....)
// SFINAE тут необязательно, но пусть будет
#define DISPATCH(fun_family) \
    [](auto&&... xs) TRY_RETURN(overloads::dispatch(FUN_OBJECT(fun_family), FWD(xs)...))

// объявление шаблона функции - фасада к диспетчеру семейства функций с приоритетами
// SFINAE тут также необязательно, но пусть будет
#define DEFINE_DISPATCH(fun_face, fun_family) \
    auto fun_face(auto&&... xs) TRY_RETURN(overloads::dispatch(FUN_OBJECT(fun_family), FWD(xs)...))

namespace overloads {

// тэг приоритета
template<int P> struct prio{ static constexpr int value = P; };
// диапазон поддержанных приоритетов
// (чем больше значение, тем приоритетнее)
constexpr int prio_max = 100;
constexpr int prio_min = 0;
constexpr int prio_error = -1;

namespace details {
// эти функции нужны исключительно для того, чтобы вывести тип результата!
// f - полиморфная функция (prio<N>,.....)
// p - prio<N>
auto find_prio_impl(auto f, auto p, auto&&... xs) {
    if constexpr(requires{f(p, FWD(xs)...);}) return p;
    else if constexpr(p.value > prio_min) return find_prio_impl(f, prio<p.value-1>{}, FWD(xs)...);
    else return prio<prio_error>{};
}
auto find_prio(auto f, auto&&... xs) {
    return find_prio_impl(f, prio<prio_max>{}, FWD(xs)...);
}
}  // namespace details

// диспетчер полиморфной функции
// подставляющий наибольший подходящий приоритет для данного набора аргументов
// SFINAE - если приоритет не найден (его значение - prio_error)
auto dispatch(auto f, auto&&... xs)
requires (decltype(details::find_prio(f, FWD(xs)...))::value != prio_error) {
    return f(decltype(details::find_prio(f, FWD(xs)...)){}, FWD(xs)...);
}

}  // namespace overloads

//////////////
// пример кода

using overloads::prio;
int fun(prio<0>, auto&&...) { return __LINE__; }

int fun(prio<5>, int) { return __LINE__; }
int fun(prio<5>, const char*) { return __LINE__; }

int fun(prio<10>, int&) { return __LINE__; }
int fun(prio<10>) { return __LINE__; }  // перегрузка с другим количеством аргументов

int fun(prio<20>, short) { return __LINE__; }
int fun(prio<20>, short&) { return __LINE__; }
int fun(prio<20>, long&) { return __LINE__; }

int fun(prio<30>, int, int) { return __LINE__; }

DEFINE_DISPATCH(do_fun, fun)

#include <iostream>

#define TEST(expr)  std::cout << #expr << " = " << (expr) << std::endl;

int main() {
    TEST(do_fun());
    TEST(do_fun(1));
    TEST(do_fun(""));
    int x;
    TEST(do_fun(x));
    long y;
    TEST(do_fun(y));
    float z;
    TEST(do_fun(z));
    TEST(do_fun(1, 2));
    short t;
    TEST(do_fun(t));
}
