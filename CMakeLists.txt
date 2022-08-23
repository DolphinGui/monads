cmake_minimum_required(VERSION 3.12)

cmake_policy(SET CMP0077 NEW)

project(monads CXX C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(MTX_MONAD_BUILD_TESTS "builds tests" ON)

include(FetchContent)
include(CTest)
include(GNUInstallDirs)

add_library(monads INTERFACE)
target_include_directories(monads 
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(monads INTERFACE cxx_std_20)

if(MTX_MONAD_BUILD_TESTS)
  add_subdirectory(tests)
endif()

install(TARGETS monads EXPORT monadTarget
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT monadTarget
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/monads NAMESPACE mtx::)