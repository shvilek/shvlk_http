cmake_minimum_required (VERSION 2.8)
project (FINAL CXX)
set(Shvlk_src
	main.cpp
	cserver.cpp
	chandler.cpp
	cserver.h
	chandler.h
	utils.h)
add_executable(final ${Shvlk_src})
TARGET_LINK_LIBRARIES(final pthread)
target_compile_features(final PRIVATE cxx_range_for)
