#include <type_traits>


#define CRITERIA_FROM_SIMPLE_CONCEPT(SimpleConcept) \
struct is_##SimpleConcept##_traits { \
    template<class T> using criteria = std::bool_constant<SimpleConcept<T>>; \
}; \
template<class T> using is_##SimpleConcept = is_##SimpleConcept##_traits::criteria<T>;

#define WITHOUT_PARENTHESES(...) __VA_ARGS__

#define CRITERIA_FROM_PARAMETRIZED_CONCEPT(Concept, FormalParams, ActualParams) \
template<WITHOUT_PARENTHESES FormalParams> \
struct is_##Concept##_traits { \
    template<class T> using criteria = std::bool_constant<Concept<T, WITHOUT_PARENTHESES ActualParams>>; \
};


template<class T> concept Simple = true;
template<class T, class A, int B> concept Complex = true;

CRITERIA_FROM_SIMPLE_CONCEPT(Simple)
CRITERIA_FROM_PARAMETRIZED_CONCEPT(Complex, (class A, int B), (A, B))

static_assert(Simple<void>);
static_assert(is_Simple<void>::value);
static_assert(is_Simple_traits::criteria<void>::value);

static_assert(Complex<void, int, 123>);
static_assert(is_Complex_traits<int, 123>::criteria<void>::value);
