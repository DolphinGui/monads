cmake_minimum_required(VERSION 3.12)

project(monads CXX C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
include(CTest)

add_library(monads INTERFACE)
target_include_directories(monads 
  INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(monads INTERFACE cxx_std_20)

add_subdirectory(tests)