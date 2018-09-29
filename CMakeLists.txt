project (autotest)
cmake_minimum_required (VERSION 3.2)

option (AUTOTEST_MEMCHECK "Link against libmcheck (memcheck) - disable if you're not on GLibC" ON)

if (APPLE OR UNIX)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif ()

if (APPLE OR UNIX)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic -ansi -D_XOPEN_SOURCE=600 -fdiagnostics-color=always")
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -Og")
	set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast")
endif ()

add_library (autotest STATIC autotest.c autotest-unix.c)
target_link_libraries(autotest dl)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND AUTOTEST_MEMCHECK)
	# man 3 mcheck
	# only on glibc
	target_link_libraries(autotest mcheck)
endif ()

if (BUILD_TESTING)
	enable_testing ()
	add_subdirectory ("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif ()
