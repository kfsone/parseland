include_guard (GLOBAL)

# CMake fragment to fetch and make available fmtlib and any dependencies.

include (FetchContent)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)

FetchContent_MakeAvailable(fmt)


find_package(Threads REQUIRED) # for pthread
