cmake_minimum_required(VERSION 2.8)

project(variant)
#enable_testing()

set (CMAKE_CXX_FLAGS "-g -O0 -std=c++14 -Wall -Werror")

add_executable(test_variant test_variant.cpp)
#add_test(TestVariant test_variant)

