cmake_minimum_required(VERSION 3.10)

project(cpp_fun)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the compilation flag for concepts diagnostics depth
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts-diagnostics-depth=10")


enable_testing()
find_package(GTest)


# Function to create test executable with gtest_discover_tests
function(add_test_executable name)
    set(sources ${ARGN})
    add_executable(${name} ${sources})
    target_link_libraries(${name} GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(${name})
endfunction()


add_test_executable(test_callable_concept single-files/callable_concept.cpp)
add_test_executable(test_compose single-files/compose.cpp)
add_test_executable(test_criteria_from_conctpt single-files/criteria_from_concept.cpp)
