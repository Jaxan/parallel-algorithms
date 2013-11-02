cmake_minimum_required (VERSION 2.6)

include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/include/")

option(MultiCoreBSP "Building with Multi Core BSP" OFF)
if(MultiCoreBSP)
	# setup for my mac
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	set(libs mcbsp1.1.0 m pthread)
	add_definitions( -DUSE_MCBSP )
else()
	# setup for cartesius
	set(libs bsponmpi m)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
set(libs ${libs} ${Boost_LIBRARIES})

add_subdirectory("primes")
add_subdirectory("wavelet")
