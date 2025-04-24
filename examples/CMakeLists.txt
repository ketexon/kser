add_executable(
	KSerExampleReflection
	reflection.cpp
)

add_executable(
	KSerExampleSerialization
	serialization.cpp
)

target_link_libraries(
	KSerExampleReflection
	KSer
)

target_link_libraries(
	KSerExampleSerialization
	KSer
)

set_target_properties(
	KSerExampleSerialization
	KSerExampleReflection
	PROPERTIES
		CXX_STANDARD 26
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
		EXPORT_COMPILE_COMMANDS YES
)