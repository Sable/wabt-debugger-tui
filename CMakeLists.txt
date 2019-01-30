cmake_minimum_required(VERSION 3.5)
set(CMAKE_VERBOSE_MAKEFILE ON)
project(wdb_tui)
set(WDB_TUI ${PROJECT_NAME})

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compile with -std=c++11
add_compile_options(-std=c++11)

# Inlucde header files
include_directories(include)

# Add wabt-debugger project
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/wabt-debugger)

# Include wabt-debugger headers
include_directories(${WDB_INCLUDE_DIRS})

# Expose include dirs for projects using wdb_tui
get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
    set(WDB_TUI_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${WDB_INCLUDE_DIRS} PARENT_SCOPE)
endif()

# Find NCurses
find_package(Curses)
if(NOT CURSES_FOUND)
    message(FATAL_ERROR "NCurses package was not found")
endif()

# Include NCurses header files
include_directories(${CURSES_INCLUDE_DIR})

# Include CDK NCurses library
set(CDK_VERSION cdk-5.0-20180306)
set(CDK_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CDK_VERSION})
include(ExternalProject)
ExternalProject_Add(cdk_project
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/cdk-snapshots/
        CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lib/cdk-snapshots/configure --prefix=${CDK_INSTALL_DIR}
        BUILD_COMMAND make
        INSTALL_COMMAND make install
)

# Import cdk static library as cdk
add_library(cdk STATIC IMPORTED)
set_property(TARGET cdk PROPERTY IMPORTED_LOCATION ${CDK_INSTALL_DIR}/lib/libcdk.a)

# Add dependency to trigger building cdk_project
add_dependencies(cdk cdk_project)

# Include CDK header files
include_directories(${CDK_INSTALL_DIR}/include)

# Generate executable
file(GLOB_RECURSE PROJECT_SOURCE_FILES src/main.cpp src/display/*.cpp)

# Add wabt dependency
add_executable(${WDB_TUI} ${PROJECT_SOURCE_FILES} ${HOST_FUNCTIONS_FILE})

# Link libraries to target
target_link_libraries(${WDB_TUI} ${CURSES_LIBRARIES} cdk form menu panel wdb)

# Default stubs
if (NOT DEFINED HOST_FUNCTIONS_STUBS)
    set(HOST_FUNCTIONS_STUBS_NAME ${WDB_TUI}_host_functions_stubs)
    add_library(${HOST_FUNCTIONS_STUBS_NAME} src/stubs/host_functions_stubs.cpp)
    target_link_libraries(${WDB_TUI} ${HOST_FUNCTIONS_STUBS_NAME})
else()
    target_link_libraries(${WDB_TUI} ${HOST_FUNCTIONS_STUBS})
endif()
