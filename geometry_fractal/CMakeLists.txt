
add_executable(geometry_fractal
	main.cpp fractal.h fractal.cpp)

set_property(TARGET geometry_fractal PROPERTY CXX_STANDARD 17)
target_link_libraries(geometry_fractal imgui glfw)

if(WIN32)
	target_link_libraries(geometry_fractal opengl32 glu32)
endif()
if(UNIX)
	target_link_libraries(geometry_fractal GL GLU)
	target_link_libraries(geometry_fractal stdc++)
endif()
