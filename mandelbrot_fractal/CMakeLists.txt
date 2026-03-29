
add_executable(mandelbrot_fractal
	main.cpp mandelbrot_plain.cpp "large_number.h" "large_number.cpp" "mandelbrot.h" "palette.h")

set_property(TARGET mandelbrot_fractal PROPERTY CXX_STANDARD 17)
target_link_libraries(mandelbrot_fractal imgui glfw)

if(WIN32)
	target_link_libraries(mandelbrot_fractal opengl32 glu32)
endif()
if(UNIX)
	target_link_libraries(mandelbrot_fractal GL GLU)
	target_link_libraries(mandelbrot_fractal stdc++)
endif()
