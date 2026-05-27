#pragma once

#include <type_traits>
#include <concepts>
#include <utility>

// thanks rg45 @ rsdn

namespace Callable_detail
{
template <typename...T>
struct Inherited : std::decay_t<T>... {};
} // namespace Callable_detail

template <typename> struct IsFreeFunction : std::false_type{};
template<typename R, typename...A> struct IsFreeFunction<R(A...)> : std::true_type{};
template <typename T> struct IsFreeFunction<T*> : IsFreeFunction<T>{};

template <typename T>
concept FreeFunction = IsFreeFunction<std::decay_t<T>>::value;

template <typename T>
concept CallableObject =
   std::is_class_v<std::decay_t<T>>
   and (
      not requires { &Callable_detail::Inherited<T, decltype([]{})>::operator(); }
      // Workaround to the bug of VS 2026.
      // Actually this check is redundant.
      or requires {&std::decay_t<T>::operator();}
   );

template <typename T>
concept Callable = FreeFunction<T> or CallableObject<T>;

template <typename T>
concept NotCallable = not Callable<T>;
