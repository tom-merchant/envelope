cmake_minimum_required(VERSION 3.13)
project(envelope C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

add_library(envelope SHARED envelope.c)
target_link_libraries(envelope pcre2-8 pcre2-posix m)
file(COPY testdata DESTINATION .)
file(COPY Ubuntu-L.ttf DESTINATION .)
file(COPY icons DESTINATION .)
#add_executable(tests tests/tests.c)
#add_dependencies(tests envelope)
#target_link_libraries(tests cmocka envelope)
#target_compile_options(tests PUBLIC -g)
#add_test(all tests)
add_executable(envelope_editor envelope_editor.cpp ImGuiFileBrowser.cpp ImGuiEnvelopeEditor.cpp)
add_dependencies(envelope_editor envelope)
target_link_libraries(envelope_editor SDL2-2.0 GLEW imgui imgui-gl-sdl-impl GL stdc++fs envelope)
#enable_testing()