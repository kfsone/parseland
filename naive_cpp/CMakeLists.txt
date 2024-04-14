# Naive implementation of the "TypeDef" grammar in C++ with CMake.
#
# By naive, I mean there's no line tracking, and only simple implementations
# of the rules and not *too* much optimization.
#
# Copyright (C) Oliver 'kfsone' Smith, 2024 -- under MIT license terms.
#
cmake_minimum_required (VERSION 3.20)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)


# We want to distinguish between being 'the' project vs being a dependent of
# some larger project, so we want to know if we are the top-level project. This
# is so common it became a built-in variable in CMake 3.21 but we can still
# determine if with earlier versions
# credit: https://www.scivision.dev/cmake-project-is-top-level/
if (CMAKE_VERSION VERSION_LESS 3.21)
  get_property (not_top DIRECTORY PROPERTY PARENT_DIRECTORY)
  if (NOT not_top)
    set (PROJECT_IS_TOP_LEVEL true)
  endif ()
  unset (not_top)
endif ()


# Declare the project itself.
project (naive-cpp LANGUAGES CXX VERSION 0.0.3)


# Use the PARSELAND_BUILD_TESTS to enable unit tests; enable it by default if we are
# the top-level project and not someone's library.
if (PROJECT_IS_TOP_LEVEL)
	option (PARSELAND_BUILD_TESTS "Enable building of unit tests (with googletest)" ON)
else ()
	option (PARSELAND_BUILD_TESTS "Enable building of unit tests (with googletest)" OFF)
endif ()
message (STATUS "Testing: ${PARSELAND_BUILD_TESTS}")


# Additional cmake odds-and-ends
include (CMake/build-flags.cmake)
include (CMake/fmtlib.cmake)
include (CMake/google-test.cmake)


# -------------------------------------------------------------------------------------------------
# Implement the scanner itself as a library so we can test/benchmark it independently of the
# command-line application.
#
add_library (
	scanner-naive_cpp

	scanner.cpp
	scanner.h

	common.h
	token.h
	result.h
	tresult.h
)
target_link_libraries (scanner-naive_cpp PRIVATE naive_cpp-build_flags)


# -------------------------------------------------------------------------------------------------
# The dependent application that uses the scanner.
#
add_executable (
	scanner-naive_cpp-app

		app-main.cpp
		app-ast.cpp
		app-ast-helpers.cpp

		app-fwd.h
		app-ast.h
		app-ast-helpers.h
		app-definitions.h
		app-tokensequence.h
)
target_link_libraries (
	scanner-naive_cpp-app
	PRIVATE
		naive_cpp-build_flags
	PUBLIC
		fmt::fmt
		scanner-naive_cpp
)


# -------------------------------------------------------------------------------------------------
# Unit tests.
#
if (PARSELAND_BUILD_TESTS)
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
