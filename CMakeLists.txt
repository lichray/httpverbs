cmake_minimum_required(VERSION 2.8)
project(httpverbs CXX)
enable_testing()

include(CTest)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY tests)

find_package(CURL REQUIRED)

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")
endif()

include_directories(${CURL_INCLUDE_DIRS})
include_directories(include)

file(GLOB httpverbs_srcs src/*.cc)
file(GLOB tests_srcs tests/*.cc)

add_library(httpverbs STATIC ${httpverbs_srcs})
target_link_libraries(httpverbs ${CURL_LIBRARIES})

if(BUILD_TESTING)
	foreach(test_src ${tests_srcs})
		get_filename_component(test_suite ${test_src} NAME_WE)
		add_executable(${test_suite} ${test_src})
		target_link_libraries(${test_suite} httpverbs)
		add_test(${test_suite} tests/${test_suite})
	endforeach()
endif()
