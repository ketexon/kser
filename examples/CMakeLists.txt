add_executable(
	KSerExample
	main.cpp
)

target_link_libraries(
	KSerExample
	KSer
)

set_target_properties(
	KSerExample
	PROPERTIES
		CXX_STANDARD 26
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		EXPORT_COMPILE_COMMANDS YES
)