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

static double zoom = 2;
static double modelx = 0;
static double modely = 0;

enum class DragMode
{
	Disabled = 0,
	Panning = 1,
	MovingObject = 2,
};

static DragMode drag_mode = DragMode::Disabled;
static double drag_startx = -1;
static double drag_starty = -1;
static int drag_iterations = 0;
static double drag_start_modelx;
static double drag_start_modely;

void unproj(double x, double y, double &objx, double &objy)
{
	double model[16];
	double proj[16];
	int viewport[4];
	double objz;

	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluUnProject(x, viewport[3] - y, 0, model, proj, viewport, &objx, &objy, &objz);
}

void key(GLFWwindow *window, int key, int scancode, int action, int flags)
{}

void mouse(GLFWwindow *window, int button, int state, int flags)
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse)return;

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if (state == GLFW_PRESS) {
			if (flags & GLFW_MOD_CONTROL)
			{
			}
			else {
				drag_mode = DragMode::Panning;
				double x, y;
				glfwGetCursorPos(window, &x, &y);
				drag_startx = x;
				drag_starty = y;
				drag_start_modelx = modelx;
				drag_start_modely = modely;
				drag_iterations = 0;
			}
		}
		else if (state == GLFW_RELEASE) {
			drag_mode = DragMode::Disabled;
		}
		break;

	case GLFW_MOUSE_BUTTON_RIGHT:
		break;
	}
	glfwPostEmptyEvent();
}

void scroll(GLFWwindow *window, double xscroll, double yscroll)
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse)return;

	if (yscroll > 0)
	{
		zoom /= 1.05;
	}
	else
	{
		zoom *= 1.05;
	}
	glfwPostEmptyEvent();
}

void motion(GLFWwindow *window, double x, double y)
{
	switch (drag_mode) {
	case DragMode::Panning:
	{
		double startx, starty, endx, endy;
		unproj(drag_startx, drag_starty, startx, starty);
		unproj(x, y, endx, endy);
		modelx = drag_start_modelx + startx - endx;
		modely = drag_start_modely + starty - endy;
		drag_iterations++;
		break;
	}
	case DragMode::MovingObject:
		break;
	}
}

void display(GLFWwindow *window)
{
	int width;
	int height;
	glfwGetFramebufferSize(window, &width, &height);
	double model_width = 10;
	double model_height = 10;
	double model_ar = model_width / model_height;
	double ar = double(width) / double(height);

	if (ar < model_ar) {
		model_height = model_width / ar;
	}
	else {
		model_width = ar * model_height;
	}

	double model_left = modelx - model_width / 2 * zoom;
	double model_right = modelx + model_width / 2 * zoom;
	double model_top = modely - model_height / 2 * zoom;
	double model_bottom = modely + model_height / 2 * zoom;

	glViewport(0, 0, width, height);

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(model_left, model_right, model_top, model_bottom, -1e2, 1e2);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.1f, 0.1f, 0.1f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const float gridsize = 20;
	glColor3f(0.3f, 0.3f, 0.3f);
	glBegin(GL_LINES);
	for (float i = -gridsize; i <= gridsize; i += 1) {
		glVertex3f(i, -gridsize, 0);
		glVertex3f(i, gridsize, 0);
	}
	for (float i = -gridsize; i <= gridsize; i += 1) {
		glVertex3f(-gridsize, i, 0);
		glVertex3f(gridsize, i, 0);
	}
	glColor3f(0.5f, 0.5f, 0.5f);
	glVertex3f(-gridsize, 0, 0);
	glVertex3f(gridsize, 0, 0);
	glVertex3f(0, -gridsize, 0);
	glVertex3f(0, gridsize, 0);
	glEnd();

	glPushMatrix();
	glTranslatef(0.f, 0.f, 1.f);
	
	// draw geometry here

	glPopMatrix();
}

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