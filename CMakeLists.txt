cmake_minimum_required(VERSION 3.0)
project(myriel)


include (cmake/utils.cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

set(LIB_SRC
	code/common/log.cpp
	code/common/fiber.cpp
	code/common/utils.cpp
	code/common/scheduler.cpp
	code/common/config.cpp
	code/common/lexical_cast.cpp
)

add_library(myriel SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(myriel)

set(LIB_LIB
	myriel
	yaml-cpp)

add_executable(test_log test/common/test_log.cpp)
add_dependencies(test_log myriel)
force_redefine_file_macro_for_sources(test_log)
target_link_libraries(test_log ${LIB_LIB})

add_executable(test_fiber test/common/test_fiber.cpp)
add_dependencies(test_fiber myriel)
force_redefine_file_macro_for_sources(test_fiber)
target_link_libraries(test_fiber ${LIB_LIB})

add_executable(test_fiber2 test/common/test_fiber2.cpp)
add_dependencies(test_fiber2 myriel)
force_redefine_file_macro_for_sources(test_fiber2)
target_link_libraries(test_fiber2 ${LIB_LIB})

add_executable(test_scheduler test/common/test_scheduler.cpp)
add_dependencies(test_scheduler myriel)
force_redefine_file_macro_for_sources(test_scheduler)
target_link_libraries(test_scheduler ${LIB_LIB})

add_executable(test_config test/common/test_config.cpp)
add_dependencies(test_config myriel)
force_redefine_file_macro_for_sources(test_config)
target_link_libraries(test_config ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)