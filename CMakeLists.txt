cmake_minimum_required(VERSION 3.30)

project(KSer)

if(PROJECT_IS_TOP_LEVEL)
	option(KSER_BUILD_TESTS "Build tests" ON)
else()
	option(KSER_BUILD_TESTS "Build tests" OFF)
endif()

add_library(
	KSer
	INTERFACE
)

target_include_directories(
	KSer
	INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(
	KSer
	PROPERTIES
		CXX_STANDARD 26
		CXX_STANDARD_REQUIRED YES
		CXX_EXTENSIONS NO
)

if(KSER_BUILD_TESTS)
	add_subdirectory(test)
endif()