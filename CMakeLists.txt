cmake_minimum_required (VERSION 2.8)
project (FINAL CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(Shvlk_src
	main.cpp
	cserver.cpp
	chandler.cpp
	cserver.h
	chandler.h
	utils.h)
add_executable(final ${Shvlk_src})
TARGET_LINK_LIBRARIES(final pthread)
