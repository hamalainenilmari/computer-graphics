cmake_minimum_required(VERSION 3.23)
project(assignment6 VERSION 1.0 LANGUAGES CXX C)

# Specify the C++ standard
# We could use C++20 for std::format, but this requires newest MacOS so we use fmt instead.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# This is required for static linkage of the libraries
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif()

# This is a precaution we take against accidental misuses of the Eigen library,
# as documented here: https://eigen.tuxfamily.org/dox-devel/group__TopicPassingByValue.html 
# Disabling vectorization this way *should* make careless code work,
# but it's better really not to ever pass Eigen objects by value.
add_compile_options(-DEIGEN_MAX_STATIC_ALIGN_BYTES=0)

# This removes ZERO_CHECK configuration
set(CMAKE_SUPPRESS_REGENERATION true)

# Put ALL_BUILD in a more reasonable place
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add external dependencies
find_package(glfw3 REQUIRED)
find_package(imgui REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(nfd REQUIRED)
find_package(fmt REQUIRED)
find_package(unofficial-im3d REQUIRED)
find_package(implot REQUIRED)
find_package(lodepng REQUIRED)
find_package(OpenMP)
find_package(tinyobjloader REQUIRED)

set(C3100_COMMON_DEPENDENCIES glfw imgui::imgui Eigen3::Eigen nfd::nfd fmt::fmt unofficial::im3d::im3d implot::implot lodepng)

if(OpenMP_CXX_FOUND)
  set(C3100_COMMON_DEPENDENCIES ${C3100_COMMON_DEPENDENCIES} OpenMP::OpenMP_CXX)
  add_compile_options(-DCS_C3100_USE_OPENMP)
endif()

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

add_executable(assignment6 src/app.cpp
                           src/app.h
                           src/main.cpp
                           src/shadowmap.cpp
                           src/shadowmap.h
                           src/camera.cpp
                           src/camera.h
                           shaders/pixel_shader.glsl
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
                           shared_sources/image.h
                           shared_sources/Eigen.natvis)
target_link_libraries(assignment6 PRIVATE ${C3100_COMMON_DEPENDENCIES})
target_include_directories(assignment6 PRIVATE shared_sources src)
set_target_properties(assignment6 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT assignment6)
set_target_properties(assignment6 PROPERTIES
                      XCODE_GENERATE_SCHEME TRUE
                      XCODE_SCHEME_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
source_group("Assignment" FILES src/app.cpp
                                src/app.h
                                src/main.cpp
                                src/shadowmap.cpp
                                src/shadowmap.h
                                src/camera.cpp
                                src/camera.h
                                shaders/pixel_shader.glsl)
source_group("Shared infrastructure" FILES shared_sources/imgui_impl_opengl3.cpp
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
                                           shared_sources/image.h
                                           shared_sources/Eigen.natvis)
