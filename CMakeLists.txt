cmake_minimum_required(VERSION 2.8)
project(httpverbs)
enable_testing()

include(CTest)
include(CheckIncludeFiles)
include(CheckFunctionExists)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY tests)

find_package(CURL REQUIRED)
find_package(Boost 1.48.0 REQUIRED)

CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_FUNCTION_EXISTS(_stricmp HAVE__STRICMP)
configure_file(src/config.h.in ${CMAKE_SOURCE_DIR}/src/config.h)

if(BUILD_TESTING)
	find_package(PythonInterp REQUIRED)
	file(TO_NATIVE_PATH ${PYTHON_EXECUTABLE} PYTHON_CMD)
	configure_file(tests/run_test_server.in
	    tests/run_test_server @ONLY)
	configure_file(tests/run_test_server.bat.in
	    tests/run_test_server.bat)

	if(NOT WIN32)
		get_filename_component(START_SCRIPT_PATH
		    tests/run_test_server ABSOLUTE)
		get_filename_component(STOP_SCRIPT_PATH
		    tests/stop_test_server ABSOLUTE)
	else()
		get_filename_component(START_SCRIPT_PATH
		    tests/run_test_server.bat ABSOLUTE)
		get_filename_component(STOP_SCRIPT_PATH
		    tests/stop_test_server.bat ABSOLUTE)
	endif()
	configure_file(CTestCustom.cmake.in CTestCustom.cmake @ONLY)
endif()

if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")
else()
	foreach(libvar CURL_LIBRARIES)
		if(${libvar} MATCHES "curl.*\\.lib$")
			add_definitions(-DCURL_STATICLIB)
			break()
		endif()
	endforeach()

	foreach(flagvar CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
	    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
		string(REGEX REPLACE "/MD" "/MT" ${flagvar} "${${flagvar}}")
		string(REGEX REPLACE "/MDd" "/MTd" ${flagvar} "${${flagvar}}")
	endforeach()
endif()

include_directories(${CURL_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
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

file(GLOB httpverbs_hdrs include/httpverbs/*.h)
configure_file(httpverbs-config.cmake.in httpverbs-config.cmake)
export(PACKAGE httpverbs)

install(FILES ${httpverbs_hdrs} DESTINATION include/httpverbs)
install(TARGETS httpverbs EXPORT httpverbs-targets ARCHIVE DESTINATION lib)
install(EXPORT httpverbs-targets DESTINATION lib/cmake/httpverbs)
install(FILES httpverbs-config.cmake DESTINATION lib/cmake/httpverbs)
