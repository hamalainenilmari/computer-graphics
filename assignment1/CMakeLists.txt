cmake_minimum_required(VERSION 3.20)
project(assignment1 VERSION 1.0 LANGUAGES CXX)

# Specify the C++ standard
# We could use C++20 for std::format, but this requires newest MacOS so we use fmt instead.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# This is required for static linkage of the libraries
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif()

# Add external dependencies
find_package(glfw3 REQUIRED)
find_package(imgui REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(nfd REQUIRED)
find_package(fmt REQUIRED)

set(C3100_COMMON_DEPENDENCIES glfw imgui::imgui Eigen3::Eigen nfd::nfd fmt::fmt)

add_compile_options(-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM)

add_executable(assignment1 src/App.cpp
                           src/App.h
                           src/main.cpp
                           shared_sources/imgui_impl_opengl3.cpp
                           shared_sources/imgui_impl_opengl3.h
                           shared_sources/glad/gl.h
                           shared_sources/ImplementGlad.cpp
                           shared_sources/ShaderProgram.h
                           shared_sources/ShaderProgram.cpp
                           shared_sources/Timer.h
                           shared_sources/Timer.cpp
                           shared_sources/Utils.h
                           shared_sources/Utils.cpp)
target_link_libraries(assignment1 PRIVATE ${C3100_COMMON_DEPENDENCIES})
target_include_directories(assignment1 PRIVATE shared_sources src)
set_target_properties(assignment1 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT assignment1)
