#ifdef WIN32
#include <Windows.h>
#endif

#include <list>

#include <glad/gl.h>
#include <GL/glu.h>
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#include "debugging.h"
#include "mandelbrot.h"

enum Precision
{
	Single = 0,
	Double,
	Large
};

static GLuint tex;
static uint8_t *image_buf = nullptr;
static size_t image_buf_size = 0;
static int image_width = 1280;
static int image_height = 720;

static int max_iterations = 1024;
static uint32_t palette[1024];

static bool is_dragging = false;
static int drag_start_x = -1;
static int drag_start_y = -1;
static int drag_stop_x = -1;
static int drag_stop_y = -1;

static rect<float> fractal;
static std::list<rect<float>> zoom_history;

static struct progress_info prog_info;
static bool is_fractal_running = false;
static int curr_precision = Precision::Single;

constexpr int smoothed_n = 16;
static double fps = 0;
static double smoothed_fps[smoothed_n];

template <typename T> rect<T> fix_aspect_ratio(const rect<T> &model, int width, int height)
{
	double window_ar = double(width) / double(height);
	double model_width = model.x1 - model.x0;
	double model_height = model.y1 - model.y0;
	double model_ar = model_width / model_height;

	if (window_ar < model_ar) {
		model_height = model_width / window_ar;
	} else {
		model_width = window_ar * model_height;
	}

	return {model.x0, static_cast<T>(model.x0 + model_width), model.y0, static_cast<T>(model.y0 + model_height)};
}

void update_image(int width, int height)
{
	if (!image_buf || width * height * 4 > image_buf_size) {
		delete[] image_buf;
		image_buf_size = width * height * 4;
		image_buf = new uint8_t[image_buf_size];
	}

	image_width = width;
	image_height = height;

	fractal = fix_aspect_ratio(fractal, width, height);
	mandelbrot_plain(image_buf, 0, 0, image_width, image_height, image_width, fractal, max_iterations, palette);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buf);
	glDisable(GL_TEXTURE_2D);
}

void update_fractal(const rect<float> &next_fractal)
{
	fractal = next_fractal;

	is_fractal_running = true;

	mandelbrot_plain(image_buf, 0, 0, image_width, image_height, image_width, fractal, max_iterations, palette);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buf);
	glDisable(GL_TEXTURE_2D);

	is_fractal_running = false;
}

void update_fractal(int width, int height, int x0, int y0, int x1, int y1)
{
	rect<float> next_fractal;
	next_fractal.x0 = (fractal.x1 - fractal.x0) * x0 / width + fractal.x0;
	next_fractal.x1 = (fractal.x1 - fractal.x0) * x1 / width + fractal.x0;
	next_fractal.y0 = (fractal.y1 - fractal.y0) * y0 / height + fractal.y0;
	next_fractal.y1 = (fractal.y1 - fractal.y0) * y1 / height + fractal.y0;

	update_fractal(next_fractal);
}

void key(GLFWwindow *window, int key, int scancode, int action, int flags)
{
	if (key == 256 && scancode == 1) {
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
	printf("key %d %d %d %d\n", key, scancode, action, flags);
}

void mouse(GLFWwindow *window, int button, int state, int flags)
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse)
		return;

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if (state == GLFW_PRESS) {
			int width;
			int height;
			glfwGetFramebufferSize(window, &width, &height);

			double x, y;
			glfwGetCursorPos(window, &x, &y);
			drag_stop_x = drag_start_x = static_cast<int>(x);
			drag_stop_y = drag_start_y = static_cast<int>(height - y);
			is_dragging = true;
		} else if (state == GLFW_RELEASE) {
			is_dragging = false;
			if (drag_start_x == drag_stop_x && drag_start_y == drag_stop_y) {
				drag_start_x = drag_start_y = drag_stop_x = drag_stop_y = -1;
			}
		}
		break;
	}
	glfwPostEmptyEvent();
}

void motion(GLFWwindow *window, double x, double y)
{
	if (is_dragging) {
		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);
		double window_ar = double(width) / double(height);
		y = height - y;
		double model_width = abs(x - drag_start_x);
		double model_height = abs(y - drag_start_y);
		double model_ar = model_width / model_height;

		if (window_ar < model_ar) {
			model_height = model_width / window_ar;
		} else {
			model_width = window_ar * model_height;
		}

		drag_stop_x = x < drag_start_x ? static_cast<int>(drag_start_x - model_width)
		                               : static_cast<int>(drag_start_x + model_width);
		drag_stop_y = y < drag_start_y ? static_cast<int>(drag_start_y - model_height)
		                               : static_cast<int>(drag_start_y + model_height);
	}
}

void display(GLFWwindow *window)
{
	int width;
	int height;
	glfwGetFramebufferSize(window, &width, &height);

	if (image_width != width || image_height != height) {
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
	glVertex2i(1, height + 1);
	glTexCoord2f(1, 1);
	glVertex2i(width + 1, height + 1);
	glTexCoord2f(1, 0);
	glVertex2i(width + 1, 1);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	if (drag_start_x > -1 && drag_start_y > -1) {
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

void draw_ui(GLFWwindow *window)
{
	using namespace ImGui;
	if (!Begin("fractal")) {
		return;
	}

	if (Button("zoom in")) {
		if (!is_fractal_running && drag_start_x > -1 && drag_start_y > -1) {
			int width;
			int height;
			glfwGetFramebufferSize(window, &width, &height);

			zoom_history.push_back(fractal);

			update_fractal(width, height, min(drag_start_x, drag_stop_x), min(drag_start_y, drag_stop_y),
			               max(drag_start_x, drag_stop_x), max(drag_start_y, drag_stop_y));

			drag_start_x = drag_start_y = drag_stop_x = drag_stop_y = -1;
		}
	}
	if (drag_start_x == -1 && drag_start_y == -1) {
		SameLine();
		Text("nothing selected");
	}

	if (Button("zoom out")) {
		if (!zoom_history.empty()) {
			update_fractal(zoom_history.back());
			zoom_history.pop_back();
		}
	}

	if (Button("update")) {
		update_fractal(fractal);
	}

	if (Combo("Precision", &curr_precision, "single\0double\0large\0")) {
	}

	if (InputInt("Iterations", &max_iterations)) {
	}

	Text("progress %d%%", (int)prog_info.progress);
	Text("last execution time %.4lf sec", prog_info.execution_time_sec);

	double v = 0;
	for (int i = 0; i < smoothed_n; ++i)
		v += smoothed_fps[i];
	v /= smoothed_n;
	Text("fps %.2f", v);
	End();
}

uint32_t pack(double r, double g, double b)
{
	return (static_cast<uint32_t>(r * 255) & 0xFF) | ((static_cast<uint32_t>(g * 255) & 0xFF) << 8) |
	       ((static_cast<uint32_t>(b * 255) & 0xFF) << 16);
}

uint32_t hsv2rgb(double h, double s, double v)
{
	double hh, p, q, t, ff;
	long i;
	double r, g, b;

	if (s <= 0.0) { // < is bogus, just shuts up warnings
		r = v;
		g = v;
		b = v;
		return pack(r, g, b);
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
	for (int i = 0; i < palette_size; ++i) {
		palette[i] = hsv2rgb(double(i) * 360 * 8 / palette_size, 0.8, 0.8);
	}

	const float ar = float(image_width) / float(image_height);
	const float scale = 0.003f;
	fractal.x0 = -image_width / 2 * scale - 1;
	fractal.x1 = +image_width / 2 * scale - 1;
	fractal.y0 = -image_width / ar / 2 * scale;
	fractal.y1 = +image_width / ar / 2 * scale;

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
	glfwSetCursorPosCallback(window, motion);

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

	Profiler prof;
	double prev_time = prof.get();
	int smoothed_i = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		draw_ui(window);

		ImGui::Render();

		display(window);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);

		double t = prof.get();
		fps = 1.0 / (t - prev_time);
		prev_time = t;

		smoothed_fps[smoothed_i++ % smoothed_n] = fps;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	delete[] image_buf;

	return 0;
}