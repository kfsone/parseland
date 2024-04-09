include_guard (GLOBAL)

# CMake chunk to fetch google test.

if (PARSELAND_BUILD_TESTS)
	enable_testing ()

	include (FetchContent)

	# Cmake was complaining about FetchContent not having DOWNLOAD_EXTRACT_TIMESTAMP but after
	# 5 minutes of looping their documentation trying to figure out where it needs setting,
	# I decided to just turn the warning off.
	cmake_policy (SET CMP0135 NEW)

	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
	)
	set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable (googletest)
	
endif ()