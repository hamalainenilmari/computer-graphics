cmake_minimum_required(VERSION 3.23)
project(assignment2 VERSION 1.0 LANGUAGES CXX C)

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
find_package(unofficial-im3d REQUIRED)

set(C3100_COMMON_DEPENDENCIES glfw imgui::imgui Eigen3::Eigen nfd::nfd fmt::fmt unofficial::im3d::im3d)

if(!WIN32)
  find_package(OpenGL REQUIRED)
  list(
    APPEND
    C3100_COMMON_DEPENDENCIES
    # https://developer.nvidia.com/blog/linking-opengl-server-side-rendering/#glvnd_support_in_cmake
    OpenGL::OpenGL
    OpenGL::GLX
    OpenGL::EGL)
endif()

add_compile_options(-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM)

# Source files common to all assignments
set(SHARED_SOURCE_DIR shared_sources)
set(SHARED_SOURCES  ${SHARED_SOURCE_DIR}/imgui_impl_opengl3.cpp
                    ${SHARED_SOURCE_DIR}/imgui_impl_opengl3.h
                    ${SHARED_SOURCE_DIR}/glad/gl_core_33.h
                    ${SHARED_SOURCE_DIR}/glad/gl_core_33.c
                    ${SHARED_SOURCE_DIR}/glad/KHR/khrplatform.h
                    ${SHARED_SOURCE_DIR}/ShaderProgram.h
                    ${SHARED_SOURCE_DIR}/ShaderProgram.cpp
                    ${SHARED_SOURCE_DIR}/Timer.h
                    ${SHARED_SOURCE_DIR}/Timer.cpp
                    ${SHARED_SOURCE_DIR}/Utils.h
                    ${SHARED_SOURCE_DIR}/Utils.cpp
                    ${SHARED_SOURCE_DIR}/im3d_opengl33.cpp
                    ${SHARED_SOURCE_DIR}/Eigen.natvis)
message("SHARED_SOURCES: ${SHARED_SOURCES}")
add_executable(assignment2 src/app.cpp
                           src/app.h
                           src/main.cpp
                           src/camera.cpp
                           src/camera.h
                           src/curve.cpp
                           src/curve.h
                           src/parse.cpp
                           src/parse.h
                           src/subdiv.cpp
                           src/subdiv.h
                           shared_sources/imgui_impl_opengl3.cpp
                           shared_sources/imgui_impl_opengl3.h
                           shared_sources/glad/gl_core_33.h
                           shared_sources/glad/gl_core_33.c
                           shared_sources/glad/KHR/khrplatform.h
                           shared_sources/ShaderProgram.h
                           shared_sources/ShaderProgram.cpp
                           shared_sources/Timer.h
                           shared_sources/Timer.cpp
                           shared_sources/Utils.h
                           shared_sources/Utils.cpp
                           shared_sources/im3d_opengl33.cpp
                           shared_sources/Eigen.natvis)
target_link_libraries(assignment2 PRIVATE ${C3100_COMMON_DEPENDENCIES})
target_include_directories(assignment2 PRIVATE shared_sources src)
set_target_properties(assignment2 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT assignment2)
