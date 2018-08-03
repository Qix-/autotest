if (BUILD_TESTING)
	enable_testing ()

	add_executable (autotest-simple-exe simple.c)
	target_link_libraries (autotest-simple-exe autotest)
	add_test (NAME autotest-simple COMMAND "$<TARGET_FILE:autotest-simple-exe>")
endif ()
