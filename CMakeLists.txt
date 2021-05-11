# CMake 最低版本号要求
cmake_minimum_required(VERSION 2.8)

# 项目信息
project("DErpc")


MESSAGE(STATUS "current system : ${CMAKE_SYSTEM_NAME}")

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS system filesystem REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    
    MESSAGE( STATUS "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}.")
    MESSAGE( STATUS "Boost_LIB_VERSION = ${Boost_LIB_VERSION}.")

else()
    MESSAGE( WARNING "Can not find Boost")
endif()


# 设置release版本还是debug版本
if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    SET(BuildType "Release")
    MESSAGE(STATUS "Release版本")
else()
    SET(BuildType "Debug")
    MESSAGE(STATUS "Debug版本")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    MESSAGE(STATUS "current platform: Windows")
    include_directories(
        "include")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "current platform: Linux")
    include_directories(
        "/usr/local/include"
        "./include")
else()
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
endif()

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/${CMAKE_SYSTEM_NAME})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME})

add_subdirectory(src/rpc)
add_subdirectory(src/rpc_middle)
add_subdirectory(example)
