cmake_minimum_required(VERSION 3.10)

project(fractal)

add_subdirectory(glfw)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_library(imgui
	imgui/imconfig.h
	imgui/imgui.cpp
	imgui/imgui.h
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_internal.h
	imgui/imgui_widgets.cpp
	imgui/imstb_rectpack.h
	imgui/imstb_textedit.h
	imgui/imstb_truetype.h
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/backends/imgui_impl_opengl3.h
	imgui/backends/imgui_impl_opengl3.cpp
	imgui/backends/imgui_impl_glfw.h
	imgui/backends/imgui_impl_glfw.cpp
	imgui/misc/cpp/imgui_stdlib.h
	imgui/misc/cpp/imgui_stdlib.cpp
	glfw/deps/glad_gl.c)

include_directories(
	imgui
	imgui/backends
	imgui/misc/cpp
	glfw/include
	glfw/deps)

add_subdirectory(geometry_fractal)
