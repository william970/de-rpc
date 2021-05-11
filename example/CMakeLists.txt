cmake_minimum_required(VERSION 3.1)
project(example)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11")

#SET(ENABLE_SSL OFF)

#if (ENABLE_SSL)
#    add_definitions(-DCINATRA_ENABLE_SSL)
#    message(STATUS "Use SSL")
#endif()

find_package(Boost 1.75.0 COMPONENTS system filesystem serialization date_time regex REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    
    MESSAGE( STATUS "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}.")
    MESSAGE( STATUS "Boost_LIB_VERSION = ${Boost_LIB_VERSION}.")

else()
    MESSAGE( WARNING "Can not find Boost")
endif()

#MESSAGE( STATUS "Boost_LIB = ${Boost_LIBRARIES}.")
#find_package(Boost 1.75.0 REQUIRED)

add_executable(derpc_middle rpc_middle/main.cpp)
add_executable(derpc_client rpc_client_test/main.cpp)
add_executable(derpc_server rpc_server/main.cpp)


#if (ENABLE_SSL)
#    target_link_libraries(derpc_server ${Boost_LIBRARIES} -lssl -lcrypto -lpthread)
#    target_link_libraries(derpc_client ${Boost_LIBRARIES} -lssl -lcrypto -lpthread)
#else()
#    target_link_libraries(derpc_server ${Boost_LIBRARIES})
#    target_link_libraries(derpc_client ${Boost_LIBRARIES})
#endif()

target_link_libraries(derpc_middle ${Boost_LIBRARIES} rpc rpc_middle)
target_link_libraries(derpc_server ${Boost_LIBRARIES} rpc)
target_link_libraries(derpc_client ${Boost_LIBRARIES} rpc)