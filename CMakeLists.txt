cmake_minimum_required(VERSION 3.15)
project(envelope C)

set(CMAKE_C_STANDARD 99)

add_library(envelope SHARED envelope.c envelope.h)
target_link_libraries(envelope pcre2-8 pcre2-posix)
file(COPY testdata DESTINATION .)
add_executable(tests tests/tests.c)
add_dependencies(tests envelope)
target_link_libraries(tests cmocka envelope)
target_compile_options(tests PUBLIC -g)
add_test(all tests)
enable_testing()