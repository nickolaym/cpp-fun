#include <utility>
#include <tuple>
#include <type_traits>
#include <iostream>
#include <string>

template<class Fun, class... Ts>
struct remapper {

    // final case - it declares an operator with some permuted signature and calls the target properly.
    template<class... Ls> struct leaf {
      auto operator()(Fun fun, Ls... ls) const {
        return fun(std::get<Ts>(std::tuple{ls...})...);
      }
    };

    // builds a tree of inheritance.
    // LT is the set of leading types (all leaves start with LT)
    // MT is the set of missed types
    // RT is the set of rest types - it will run on them

    template<class LT, class MT, class RT> struct tree;
    // all = tree<{Ts...}, {}, {}>
    // tree<{Ls...}, {}, {Rs...}> = tree<{Ls...,R}, {}, {Rs... \ R}> for all R of Rs...
    // in details
    // tree<{Ls...}, {Ms...}, {R,Rs...}> =
    //   = tree<{Ls...,R}, {}, {Ms...,Rs...}> - that is, accept first R of Rs and go down
    //   + tree<{Ls...}, {Ms...,R}, {Rs...}> - that is, reject first R and go right
    // tree<{Ls...}, {Ms...}, {R}> =
    //   = tree<{Ls...,R}, {}, {Ms...}> - always accept the last R and go down

    // the end of the recursion: all types are leading, nothing to run about.
    template<class... Ls>
    struct  tree<std::tuple<Ls...>, std::tuple<>, std::tuple<>>
         :  leaf<Ls...>
    {
      using leaf<Ls...>::operator();
    };

    // the end of the recursion on given RT: obviously use the rest type and go down
    template<class... Ls, class... Ms, class R>
    struct  tree<std::tuple<Ls...>, std::tuple<Ms...>, std::tuple<R>>
         :  tree<std::tuple<Ls..., R>, std::tuple<>, std::tuple<Ms...>>
    {
      using tree<std::tuple<Ls..., R>, std::tuple<>, std::tuple<Ms...>>::operator();
    };

    // recursion on given RT: either accept the first type and go down, or reject and go right
    template<class... Ls, class... Ms, class R, class... Rs>
    struct  tree<std::tuple<Ls...>, std::tuple<Ms...>, std::tuple<R, Rs...>>
         :  tree<std::tuple<Ls..., R>, std::tuple<>, std::tuple<Ms..., Rs...>>
         ,  tree<std::tuple<Ls...>, std::tuple<Ms..., R>, std::tuple<Rs...>>
    {
      using tree<std::tuple<Ls..., R>, std::tuple<>, std::tuple<Ms..., Rs...>>::operator();
      using tree<std::tuple<Ls...>, std::tuple<Ms..., R>, std::tuple<Rs...>>::operator();
    };

    using all_permutations = tree<std::tuple<>, std::tuple<>, std::tuple<Ts...>>;

    template<class... Args>
    auto operator()(Fun fun, Args... args) const {
      return all_permutations{}(fun, args...);
    }
};

template<class R, class... Args>
auto make_arbitrary_order(R(*fun)(Args...)) {
  return [fun](auto... args) { return remapper<decltype(fun), Args...>{}(fun, args...); };
}

void hello(bool b, int i, float f, const char* s) {
  std::cout << "hello(" << std::boolalpha << b << ", " << i << ", " << f << ", " << s << ")" << std::endl;
}

int main() {
  auto world = make_arbitrary_order(hello);
  world(false, 123, "test", 45.67);
  world(123, "test", 45.67, (void*)nullptr);
  world("test", 123U, (void*)nullptr, 45.67f);
}
