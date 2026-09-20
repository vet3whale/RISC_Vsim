cmake_minimum_required(VERSION 3.16)

project(RISC-Vsim LANGUAGES CXX)

add_executable(RISC-Vsim
    src/main.cpp
    src/decoder/decoder.cpp
    src/decoder/i_type.cpp
    src/decoder/r_type.cpp
    src/decoder/s_type.cpp
    src/decoder/b_type.cpp
    src/decoder/j_type.cpp
)

target_include_directories(RISC-Vsim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(RISC-Vsim PRIVATE cxx_std_11)
set_target_properties(RISC-Vsim PROPERTIES CXX_EXTENSIONS OFF)
