cmake_minimum_required(VERSION 3.20)
project(ChonkyStation3MemorySmoke LANGUAGES CXX)

enable_testing()
add_executable(ps3_memory_smoke
    memory_smoke.cpp
    ../ChonkyStation3/Memory/Memory.cpp
)
target_compile_features(ps3_memory_smoke PRIVATE cxx_std_23)
target_include_directories(ps3_memory_smoke PRIVATE
    ../ChonkyStation3/Common
    ../ChonkyStation3/Memory
    ../Dependencies/Panda3DS
)
add_test(NAME memory_alloc_free_unmap COMMAND ps3_memory_smoke)
