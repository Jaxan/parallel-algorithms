file(GLOB sources *.cpp)
file(GLOB headers *.hpp)

foreach(source ${sources})
	get_filename_component(exec ${source} NAME_WE)
	add_executable(${exec} ${source} ${headers})
	target_link_libraries(${exec} ${libs})
endforeach()
