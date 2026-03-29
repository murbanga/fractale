#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include <list>
#include <algorithm>
#include <variant>

#define LARGE_NUMBERS 1

#if LARGE_NUMBERS
#define BOOST_MP_USE_QUAD
//#include <boost/multiprecision/float128.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>

using fp = std::variant<float, double, boost::multiprecision::cpp_bin_float<128>>;
#else
using fp = std::variant<float, double>;
#endif

#include <glad/gl.h>
#include <GL/glu.h>
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#include "debugging.h"
#include "mandelbrot.h"
#include "pool.h"
#include "palette.h"

using namespace std;

enum class Precision
{
	Single = 0,
	Double = 1,
	Large = 2,
};

static GLuint tex;
static Image image;

static int max_iterations = 1024;
static uint32_t palette[1024];

static bool is_dragging = false;
static Rect<int> drag = {-1, -1, -1, -1};

static Rect<fp> fractal;
static std::list<Rect<fp>> zoom_history;

static pool calc_pool;

static Profiler prof;
static struct progress_info prog_info;
static int curr_precision = static_cast<int>(Precision::Single);

constexpr int smoothed_n = 60;
static double fps = 0;
static double smoothed_fps[smoothed_n];

template <typename T> Rect<T> fix_aspect_ratio(const Rect<T> &model, int width, int height)
{
	T window_ar = T(width) / T(height);
	T model_width = model.x1 - model.x0;
	T model_height = model.y1 - model.y0;
	T model_ar = model_width / model_height;

	if (window_ar < model_ar) {
		model_height = model_width / window_ar;
	} else {
		model_width = window_ar * model_height;
	}

	return {model.x0, model.x0 + model_width, model.y0, model.y0 + model_height};
}

template <typename T> void update_fractal(Image &image, const Rect<T> &next_fractal)
{
	fractal = fix_aspect_ratio(next_fractal, image.width, image.height);

	prof.start();
	prog_info.progress_num = 0;
	prog_info.progress_den = image.height;

#if 1
	const int nthreads = min(16, (int)thread::hardware_concurrency());

	calc_pool.start(nthreads, (image.height + 1) / nthreads, [&](int i) {
		mandelbrot(image, 0, i, image.width, min(i + 1, image.height), fractal, max_iterations, palette);
		prog_info.progress_num++;
	});
#else
	mandelbrot_plain(image.buf, 0, 0, image.width, image.height, image.width, fractal, max_iterations, palette);
	prog_info.progress = 100;
#endif
	image.idx++;
}

template <typename T> void update_fractal(Image &image, const Rect<int> &d)
{
	Rect<T> next_fractal;

	if (drag.valid()) {
		next_fractal.x0 = (fractal.x1 - fractal.x0) * d.x0 / image.width + fractal.x0;
		next_fractal.x1 = (fractal.x1 - fractal.x0) * d.x1 / image.width + fractal.x0;
		next_fractal.y0 = (fractal.y1 - fractal.y0) * d.y0 / image.height + fractal.y0;
		next_fractal.y1 = (fractal.y1 - fractal.y0) * d.y1 / image.height + fractal.y0;
	} else {
		next_fractal = fractal;
	}

	update_fractal<T>(image, next_fractal);
}

void invoke_fractal(Precision precision, Image &image, const Rect<int> &drag, int n, const uint32_t *palette)
{
	switch (precision) {
	case Precision::Single:
		update_fractal<float>(image, drag);
		break;
	case Precision::Double:
		// update_fractal<double>(image, drag);
		break;
#if LARGE_NUMBERS
	case Precision::Large:
		break;
#endif
	}
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
			drag.x1 = drag.x0 = static_cast<int>(x);
			drag.y1 = drag.y0 = static_cast<int>(height - y);
			is_dragging = true;
		} else if (state == GLFW_RELEASE) {
			is_dragging = false;
			if (drag.x0 == drag.x1 && drag.y0 == drag.y1) {
				drag.x0 = drag.y0 = drag.x1 = drag.y1 = -1;
			}
		}
		break;
	}
	glfwPostEmptyEvent();
}

void motion(GLFWwindow *window, double raw_x, double raw_y)
{
	if (is_dragging) {
		int x = static_cast<int>(raw_x);
		int y = static_cast<int>(raw_y);
		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);
		double window_ar = double(width) / double(height);
		y = height - y;
		int model_width = abs(x - drag.x0);
		int model_height = abs(y - drag.y0);
		double model_ar = double(model_width) / double(model_height);

		if (window_ar < model_ar) {
			model_height = static_cast<int>(model_width / window_ar);
		} else {
			model_width = static_cast<int>(window_ar * model_height);
		}

		drag.x1 = x < drag.x0 ? drag.x0 - model_width : drag.x0 + model_width;
		drag.y1 = y < drag.y0 ? drag.y0 - model_height : drag.y0 + model_height;
	}
}

void update_texture(const Image &image)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.buf);
	glDisable(GL_TEXTURE_2D);
}

void display(GLFWwindow *window)
{
	int width;
	int height;
	glfwGetFramebufferSize(window, &width, &height);

	if (image.width != width || image.height != height) {
		if (!image.buf || width * height * 4 > image.buf_size) {
			delete[] image.buf;
			image.buf_size = width * height * 4;
			image.buf = new uint8_t[image.buf_size];
		}

		image.width = width;
		image.height = height;

		update_fractal<float>(image, drag.normalize());
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

	if (drag.valid()) {
		glColor3f(1, 1, 1);
		glBegin(GL_LINE_LOOP);
		glVertex2i(drag.x0, drag.y0);
		glVertex2i(drag.x0, drag.y1);
		glVertex2i(drag.x1, drag.y1);
		glVertex2i(drag.x1, drag.y0);
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
		if (drag.valid()) {
			int width;
			int height;
			glfwGetFramebufferSize(window, &width, &height);

			zoom_history.push_back(fractal);

			update_fractal<float>(image, drag.normalize());

			drag.x0 = drag.y0 = drag.x1 = drag.y1 = -1;
		}
	}
	if (!drag.valid()) {
		SameLine();
		Text("nothing selected");
	}

	if (Button("zoom out")) {
		if (!zoom_history.empty()) {
			update_fractal(image, zoom_history.back());
			zoom_history.pop_back();
		}
	}

	if (Button("update")) {
		update_fractal(image, fractal);
	}

	const char *items =
#if LARGE_NUMBERS
	    "single\0double\0large\0"
#else
	    "single\0double\0"
#endif
	    ;

	if (Combo("Precision", &curr_precision, items)) {
	}

	if (InputInt("Iterations", &max_iterations)) {
	}

	Separator();

	// Text("progress %d%%", prog_info.progress_num * 100 / prog_info.progress_den);
	char str[256];
	snprintf(str, sizeof(str), "progress %d%%", prog_info.progress_num * 100 / prog_info.progress_den);
	ProgressBar((float)prog_info.progress_num / prog_info.progress_den, {0, 0}, str);
	Text("last execution time %.4lf sec", prog_info.execution_time_sec);

	double v = 0;
	for (int i = 0; i < smoothed_n; ++i)
		v += smoothed_fps[i];
	v /= smoothed_n;
	Text("fps %.2f", v);
	End();
}

int main(int argc, char **argv)
{
	int palette_size = sizeof(palette) / sizeof(palette[0]);
	for (int i = 0; i < palette_size; ++i) {
		palette[i] = hsv2rgb(double(i) * 360 * 8 / palette_size, 0.8, 0.8);
	}

	image.width = 1280;
	image.height = 720;
	image.buf_size = image.width * image.height * 4;
	image.buf = new uint8_t[image.buf_size];

	const float ar = float(image.width) / float(image.height);
	const float scale = 0.003f;
	fractal.x0 = -image.width / 2 * scale - 1;
	fractal.x1 = +image.width / 2 * scale - 1;
	fractal.y0 = -image.width / ar / 2 * scale;
	fractal.y1 = +image.width / ar / 2 * scale;

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	GLFWwindow *window = glfwCreateWindow(image.width, image.height, "fractale", nullptr, nullptr);
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

	update_fractal<float>(image, {0, 0, image.width, image.height});

	double prev_time = Profiler::get();
	int smoothed_i = 0;
	int prev_image_idx = -1;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		draw_ui(window);

		ImGui::Render();

		if (calc_pool.is_finished() && image.idx != prev_image_idx) {
			update_texture(image);
			calc_pool.join();
			prog_info.execution_time_sec = prof.elapsed_time();
			prev_image_idx = image.idx;
		}

		display(window);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);

		double t = Profiler::get();
		fps = 1.0 / (t - prev_time);
		prev_time = t;
		smoothed_fps[smoothed_i++ % smoothed_n] = fps;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	delete[] image.buf;

	return 0;
}
