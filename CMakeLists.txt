cmake_minimum_required (VERSION 3.1.0)
project (Chat)

set(CMAKE_CXX_FLAGS "-Wall -Werror -std=c++14 -stdlib=libc++")

add_executable (chat_client chat_client.cpp chat_session.h)
add_executable (chat_server chat_server.cpp chat_session.h)

find_package (Boost 1.59.0 COMPONENTS system REQUIRED)
find_package (Threads REQUIRED)

target_link_libraries (chat_client ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries (chat_server ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
