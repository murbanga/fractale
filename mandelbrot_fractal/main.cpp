#ifdef WIN32
#include <Windows.h>
#endif

#include <glad/gl.h>
#include <GL/glu.h>
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

static GLuint tex;
static uint8_t *image_buf = nullptr;
static int image_width = 1280;
static int image_height = 720;
//static int tex_width = 2048;
//static int tex_height = 1024;

static int max_iterations = 1024;
static uint32_t palette[1024];

static float fractal_x0 = -2.5f;
static float fractal_x1 = 1.0f;
static float fractal_y0 = -1.0f;
static float fractal_y1 = 1.0f;

static int drag_start_x = -1;
static int drag_start_y = -1;
static int drag_stop_x = -1;
static int drag_stop_y = -1;

extern int mandelbrot_plain(uint8_t *buf, int width, int height, float x0, float y0, float x1, float y1, int n, const uint32_t *palette);

void update_image(int width, int height)
{
	if (width > image_width || height > image_height)
	{
		delete[] image_buf;
		size_t size = width * height * 4;
		image_buf = new uint8_t[size];
	}

	image_width = width;
	image_height = height;

#if 0
	memset(image_buf, 0, size);

	for (int y = 0; y < image_height; ++y)
	{
		for (int x = 0; x < image_width * 4; x += 4)
		{
			int idx = y * image_width * 4 + x;
			*(uint32_t *)(image_buf + idx) = palette[(x/4 + y) % 1024];
			//image_buf[idx + 0] = 0;
			//image_buf[idx + 1] = y & 1 ? y : 0;
			//image_buf[idx + 2] = (x / 4) & 1 ? x / 4 : 0;
			//image_buf[idx + 3] = 255;
		}
	}
#else
	mandelbrot_plain(image_buf, image_width, image_height, fractal_x0, fractal_y0, fractal_x1, fractal_y1, max_iterations, palette);
#endif

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buf);
	glDisable(GL_TEXTURE_2D);
}

void key(GLFWwindow *window, int key, int scancode, int action, int flags)
{
	if (key == 256 && scancode == 1)
	{
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
	printf("key %d %d %d %d\n", key, scancode, action, flags);
}

void mouse(GLFWwindow *window, int button, int state, int flags)
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse)return;

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if (state == GLFW_PRESS) {

			double x, y;
			glfwGetCursorPos(window, &x, &y);

		}
		else if (state == GLFW_RELEASE) {
		}
		break;
	}
	glfwPostEmptyEvent();
}

void display(GLFWwindow *window)
{
	//double x, y;
	//glfwGetCursorPos(window, &x, &y);
	//double mousex, mousey;
	//unproj(x, y, mousex, mousey);

	int width;
	int height;
	glfwGetFramebufferSize(window, &width, &height);

	if (width != image_width || height != image_height)
	{
		update_image(width, height);
	}

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1e2, 1e2);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.1f, 0.1f, 0.1f, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2i(1, 1);
	glTexCoord2f(0, 1);
	glVertex2i(1, height);
	glTexCoord2f(1, 1);
	glVertex2i(width, height);
	glTexCoord2f(1, 0);
	glVertex2i(width, 1);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	if (drag_start_x > -1 && drag_start_y > -1)
	{
		glColor3f(1, 1, 1);
		glBegin(GL_LINE_LOOP);
		glVertex2i(drag_start_x, drag_start_y);
		glVertex2i(drag_start_x, drag_stop_y);
		glVertex2i(drag_stop_x, drag_stop_y);
		glVertex2i(drag_stop_x, drag_start_y);
		glEnd();
	}
}

void glfw_error_callback(int error, const char *description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void draw_ui()
{
	using namespace ImGui;
}

uint32_t pack(double r, double g, double b)
{
	return (static_cast<uint32_t>(r * 255) & 0xFF) |
		((static_cast<uint32_t>(g * 255) & 0xFF) << 8) |
		((static_cast<uint32_t>(b * 255) & 0xFF) << 16);
}

uint32_t hsv2rgb(double h, double s, double v)
{
	double hh, p, q, t, ff;
	long i;
	double r, g, b;

	if (s <= 0.0) {       // < is bogus, just shuts up warnings
		r = v;
		g = v;
		b = v;
		return pack(r,g,b);
	}
	hh = h - ((int)h / 360 * 360);
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = v * (1.0 - s);
	q = v * (1.0 - (s * ff));
	t = v * (1.0 - (s * (1.0 - ff)));

	switch (i) {
	case 0:
		r = v;
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = v;
		b = p;
		break;
	case 2:
		r = p;
		g = v;
		b = t;
		break;

	case 3:
		r = p;
		g = q;
		b = v;
		break;
	case 4:
		r = t;
		g = p;
		b = v;
		break;
	case 5:
	default:
		r = v;
		g = p;
		b = q;
		break;
	}
	return pack(r, g, b);
}

int main(int argc, char **argv)
{
	int palette_size = sizeof(palette) / sizeof(palette[0]);
	for (int i = 0; i < palette_size; ++i)
	{
		palette[i] = hsv2rgb(double(i) * 720 / palette_size, 0.8, 0.8);
	}

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	GLFWwindow *window = glfwCreateWindow(image_width, image_height, "fractale", nullptr, nullptr);
	if (!window)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetMouseButtonCallback(window, mouse);
	glfwSetKeyCallback(window, key);

	gladLoadGL(glfwGetProcAddress);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	io.FontDefault = io.Fonts->AddFontDefault();

	glGenTextures(1, &tex);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glDisable(GL_TEXTURE_2D);

	update_image(image_width, image_height);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		draw_ui();

		ImGui::Render();

		display(window);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	delete[] image_buf;

	return 0;
}