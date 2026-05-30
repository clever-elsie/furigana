cmake_minimum_required(VERSION 3.25)

set(JPP "/home/elsie/lab/jpp/include")
set(LIB_DIR "/usr/local/bin/lib64")
set(CMAKE_C_COMPILER /usr/local/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/local/bin/g++)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH ${LIB_DIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

project(furigana)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -freflection -finput-charset=utf-8 -fexec-charset=utf-8")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -G -O0")
  add_definitions(-DDEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
endif()

find_package(CURL REQUIRED)

include_directories(include ${JPP} ${CURL_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")


add_executable(furigana ${SOURCES})

target_link_options(furigana PRIVATE "-Wl,-rpath,${LIB_DIR}")

target_link_libraries(furigana PRIVATE CURL::libcurl)