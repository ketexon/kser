include(FetchContent)

FetchContent_Declare(
	KTest
	GIT_REPOSITORY https://github.com/ketexon/KTest
	GIT_TAG main
)

FetchContent_MakeAvailable(KTest)

add_executable(
	KSerTest
	main.cpp
)

target_link_libraries(
	KSerTest
	KSer
	KTestWithMain
)

set_target_properties(
	KSerTest
	PROPERTIES
		CXX_STANDARD 26
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		EXPORT_COMPILE_COMMANDS YES
)