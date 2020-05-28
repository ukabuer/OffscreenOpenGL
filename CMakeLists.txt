cmake_minimum_required(VERSION 3.10)

project(
    OffscreenOpenGL
    LANGUAGES C CXX
    VERSION 0.1.0
)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(OpenGL REQUIRED COMPONENTS EGL)

add_library(glad OBJECT third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include/)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

add_executable(eglinfo src/eglinfo.c)
target_link_libraries(
    eglinfo
    PRIVATE
    OpenGL::EGL
    glad
)

add_executable(triangle src/triangle.cpp)
target_include_directories(triangle PRIVATE src)
target_link_libraries(triangle PRIVATE OpenGL::EGL glad -static-libstdc++)
target_compile_features(triangle PRIVATE cxx_std_14)
