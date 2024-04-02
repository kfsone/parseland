include_guard (GLOBAL)

# CMake chunk to fetch google test.

if (PARSELAND_BUILD_TESTS)
	enable_testing ()

	include (FetchContent)

	FetchContent_Declare(
		googletest
		URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
	)
	set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable (googletest)
	
endif ()