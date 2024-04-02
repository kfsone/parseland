include_guard (GLOBAL)

# Rather than mucking about with flag strings, we can use a build target to carry our
# settings/config/attributes around for us.

add_library (naive_cpp-build_flags INTERFACE)
# Compiler options: Crank up the warnings.

if (MSVC)
	target_compile_options (naive_cpp-build_flags INTERFACE /W3 /WX)
else ()
  target_compile_options (naive_cpp-build_flags INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif ()
