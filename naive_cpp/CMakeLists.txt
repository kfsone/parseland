# Naive implementation of the "TypeDef" grammar in C++ with CMake.
#
# By naive, I mean there's no line tracking, and only simple implementations
# of the rules and not *too* much optimization.
#
# Copyright (C) Oliver 'kfsone' Smith, 2024 -- under MIT license terms.
#

cmake_minimum_required (VERSION 3.20)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

if (PROJECT_IS_TOP_LEVEL)
	option (PARSELAND_BUILD_TESTS "Enable building of unit tests (with googletest)" ON)
else ()
	option (PARSELAND_BUILD_TESTS "Enable building of unit tests (with googletest)" ON)
endif ()

project (naive-cpp LANGUAGES CXX VERSION 0.0.2)

add_library (
	scanner-naive_cpp

	scanner.cpp
	scanner.h

	common.h
	token.h
	tresult.h
)

if (MSVC)
  target_compile_options (scanner-naive_cpp PRIVATE /W3 /WX)
else ()
  target_compile_options (scanner-naive_cpp PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif ()

# Mm. Lets do some tests and benchmarks. We'll use google test.
if (PARSELAND_BUILD_TESTS)
	enable_testing ()
	include (FetchContent)
		FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
	)
	set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable (googletest)
	
	add_executable (
		scanner-naive_cpp-test

		scanner_test.cpp
	)

	target_link_libraries (
		scanner-naive_cpp-test

		PRIVATE
		GTest::gtest_main
		scanner-naive_cpp
	)

	include (GoogleTest)
	gtest_discover_tests (scanner-naive_cpp-test)
endif ()
