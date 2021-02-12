#ifdef WIN32
#include <Windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "imgui_stdlib.h"

void key(GLFWwindow *window, int key, int scancode, int action, int flags)
{}

void mouse(GLFWwindow *window, int button, int state, int flags)
{}

void scroll(GLFWwindow *window, double xscroll, double yscroll)
{}

void motion(GLFWwindow *window, double x, double y)
{}

void display(GLFWwindow *window)
{}

void glfw_error_callback(int error, const char *description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int argc, char **argv)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	GLFWwindow *window = glfwCreateWindow(1280, 720, "Road viz", nullptr, nullptr);
	if (!window)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetMouseButtonCallback(window, mouse);
	glfwSetScrollCallback(window, scroll);
	glfwSetCursorPosCallback(window, motion);
	glfwSetKeyCallback(window, key);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	io.FontDefault = io.Fonts->AddFontDefault();

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Render();

		display(window);

		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}