cmake_minimum_required (VERSION 3.20)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

project (naive-cpp LANGUAGES CXX VERSION 0.0.1)

add_library (
	naive-cpp-parser

	naive.cpp

	common.h
	parser.h
	token.h
	tresult.h
)

if (MSVC)
  target_compile_options (naive-cpp-parser PRIVATE /W3 /WX)
else ()
  target_compile_options (naive-cpp-parser PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif ()

# Mm. Lets do some tests and benchmarks.

Include (FetchContent)
include (FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable (googletest)
  

enable_testing ()

add_executable (
	naive-cpp-parser-test
	naive_test.cpp
)

target_link_libraries (
	naive-cpp-parser-test

	PRIVATE
	GTest::gtest_main
	naive-cpp-parser
)

include (GoogleTest)
gtest_discover_tests (naive-cpp-parser-test)
