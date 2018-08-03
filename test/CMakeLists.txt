if (BUILD_TESTING)
	enable_testing ()

	add_executable (autotest-test1-exe test1.c)
	target_link_libraries (autotest-test1-exe autotest)
	add_test (NAME autotest-test1 COMMAND "$<TARGET_FILE:autotest-test1-exe>")
endif ()
