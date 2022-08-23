FetchContent_Declare(fmt
GIT_REPOSITORY https://github.com/fmtlib/fmt.git
GIT_TAG c48be439f1ae03f2726e30ac93fce3a667dc4be2)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest fmt)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE monads fmt::fmt)
add_test(NAME example COMMAND example)

include(GoogleTest)
add_executable(maybe maybe.cpp)
target_link_libraries(maybe PRIVATE monads GTest::gtest_main)
gtest_discover_tests(maybe)
add_executable(exceptions exceptions.cpp)
target_link_libraries(exceptions PRIVATE monads GTest::gtest_main)
gtest_discover_tests(exceptions)
add_executable(composition composition.cpp)
target_link_libraries(composition PRIVATE monads GTest::gtest_main)
gtest_discover_tests(composition)