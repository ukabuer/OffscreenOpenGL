cmake_minimum_required(VERSION 3.11)

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
