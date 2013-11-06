file(GLOB sources *.cpp)
file(GLOB headers *.hpp)

find_package(PNG REQUIRED)
include_directories(SYSTEM ${PNG_INCLUDE_DIRS})
set(libs ${libs} ${PNG_LIBRARIES})

foreach(source ${sources})
	get_filename_component(exec ${source} NAME_WE)
	add_executable(${exec} ${source} ${headers})
	target_link_libraries(${exec} ${libs})
endforeach()
