cmake_minimum_required (VERSION 3.4)
set (PROJECT_NAME libgmsd)
set (LIB_NAME libgmsd)
set (LIB_TYPE STATIC)
set (TEST_NAME gmsd_tests)
set (BIN_NAME gmsd)
set (CMAKE_BUILD_TYPE Release)
set (GCC_COVERAGE_LINK_FLAGS "-lm")
set (CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

project (${PROJECT_NAME})
include_directories(include)
include_directories(3rdparty)

# Common 3rdparty for exe and test app
file(GLOB 3RDPARTY_SOURCES "3rdparty/*.c")

# LIB
file(GLOB LIB_SOURCES "src/*.c")
add_library(${LIB_NAME} ${LIB_TYPE} ${LIB_SOURCES})

# EXE
file(GLOB BIN_SOURCES "bin/*.c")
add_executable(${BIN_NAME} ${BIN_SOURCES} ${3RDPARTY_SOURCES})
target_link_libraries( ${BIN_NAME} ${LIB_NAME} )

# CTEST APP
enable_testing()

file(GLOB TEST_SOURCES "test/*.c")
file(GLOB TEST_FILES "test/*.bmp")
  
add_executable(${TEST_NAME} ${TEST_SOURCES} ${3RDPARTY_SOURCES})
target_link_libraries( ${TEST_NAME} ${LIB_NAME} )

add_custom_command(
        TARGET ${TEST_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${TEST_FILES}
                ${CMAKE_CURRENT_BINARY_DIR}				
				)
add_test( NAME ${TEST_NAME} COMMAND ${TEST_NAME} )
