#ifdef WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include <list>
#include <algorithm>

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

Palette palette;

static bool is_dragging = false;
static Rect<int> drag = {-1, -1, -1, -1};

static Rect<fp> fractal;
static std::list<Rect<fp>> zoom_history;

static pool calc_pool;

static Profiler prof;
static struct progress_info prog_info;
static int precision = static_cast<int>(Precision::Single);

constexpr int smoothed_n = 60;
static double fps = 0;
static double smoothed_fps[smoothed_n];

template <typename T> Rect<fp> f(Precision p, const Rect<T> &s)
{
	switch (p) {
	case Precision::Single:
		return {e<float>(s.x0), e<float>(s.x1), e<float>(s.y0), e<float>(s.y1)};
	case Precision::Double:
		return {e<double>(s.x0), e<double>(s.x1), e<double>(s.y0), e<double>(s.y1)};
#if LARGE_NUMBERS
	case Precision::Large:
		return {(float128)s.x0, (float128)s.x1, (float128)s.y0, (float128)s.y1};
#endif
	}
	assert(false);
	return {};
}

template <typename T> Rect<T> fix_aspect_ratio(const Rect<T> &model, int width, int height)
{
	/*double window_ar = double(width) / double(height);
        T model_width = model.x1 - model.x0;
	T model_height = model.y1 - model.y0;
	T model_ar = model_width / model_height;

	if (window_ar < model_ar) {
		model_height = model_width / window_ar;
	} else {
		model_width = window_ar * model_height;
	}

	return {model.x0, model.x0 + model_width, model.y0, model.y0 + model_height};*/
	return model;
}

template <typename T> Rect<fp> update_fractal(Image &image, const Rect<T> &next_fractal)
{
	Rect<T> fractal = fix_aspect_ratio(next_fractal, image.width, image.height);

	printf("running fractal of [%s,%s,%s,%s]\nto [%d,%d]\n", fptostr(fractal.x0).c_str(),
	       fptostr(fractal.x1).c_str(), fptostr(fractal.y0).c_str(), fptostr(fractal.y1).c_str(), image.width,
	       image.height);

	prof.start();
	prog_info.progress_num = 0;
	prog_info.progress_den = image.height;

#if 1
	const int nthreads = min(16, (int)thread::hardware_concurrency());

	calc_pool.join();
	calc_pool.start(nthreads, (image.height + 1) / nthreads, [&image, fractal](int i) {
		mandelbrot(image, 0, i, image.width, min(i + 1, image.height), fractal, max_iterations, palette);
		prog_info.progress_num++;
	});
#else
	mandelbrot(image, 0, 0, image.width, image.height, fractal, max_iterations, palette);
	prog_info.progress_den = image.height;
#endif
	image.idx++;

	return {fractal.x0, fractal.x1, fractal.y0, fractal.y1};
}

template <typename T> Rect<fp> update_fractal(Image &image, const Rect<int> &d, const Rect<fp> &fractal)
{
	Rect<T> next_fractal, old_fractal = collapse<T>(fractal);

	if (drag.valid()) {
		next_fractal.x0 = (old_fractal.x1 - old_fractal.x0) * d.x0 / image.width + old_fractal.x0;
		next_fractal.x1 = (old_fractal.x1 - old_fractal.x0) * d.x1 / image.width + old_fractal.x0;
		next_fractal.y0 = (old_fractal.y1 - old_fractal.y0) * d.y0 / image.height + old_fractal.y0;
		next_fractal.y1 = (old_fractal.y1 - old_fractal.y0) * d.y1 / image.height + old_fractal.y0;
	} else {
		next_fractal = old_fractal;
	}

	return update_fractal(image, next_fractal);
}

Rect<fp> convert(const Rect<fp> &r, Precision newp, Precision oldp)
{
	switch (oldp) {
	case Precision::Single:
		return f(newp, collapse<float>(r));
	case Precision::Double:
		return f(newp, collapse<double>(r));
#if LARGE_NUMBERS
	case Precision::Large:
		return f(newp, collapse<float128>(r));
#endif
	}
	assert(false);
	return {};
}

Rect<fp> invoke_fractal(Precision precision, Image &image, const Rect<int> &drag, const Rect<fp> fractal)
{
	switch (precision) {
	case Precision::Single:
		return update_fractal<float>(image, drag, fractal);
		break;
	case Precision::Double:
		return update_fractal<double>(image, drag, fractal);
		break;
#if LARGE_NUMBERS
	case Precision::Large:
		return update_fractal<float128>(image, drag, fractal);
		break;
#endif
	}
	assert(false);
	return {};
}

void key(GLFWwindow *window, int key, int scancode, int action, int flags)
{
	// esc
	if (key == 256 && scancode == 1) {
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
#if 0
	constexpr int move_offset = 100;
	Rect<int> move{ -1, -1, -1, -1 };

	// down
	else if (key == 264 && scancode == 336) {
		printf("down\n");
		move.x0 = 0;
		move.y0 = move_offset;
		move.x1 = image.width;
		move.y1 = image.height + move_offset;
	}
	// up
	else if (key == 265 && scancode == 328) {
		printf("up\n");
		move.x0 = 0;
		move.y0 = -move_offset;
		move.x1 = image.width;
		move.y1 = image.height - move_offset;
	}
	// left
	else if (key == 263 && scancode == 331) {
		printf("left\n");
	}
	// eight
	else if (key == 262 && scancode == 333) {
		printf("right\b");
	} else {
		printf("unknown key %d %d %d %d\n", key, scancode, action, flags);
	}

	if (move.valid())
	{
		fractal = invoke_fractal(static_cast<Precision>(precision), image, move, fractal);
		glfwPostEmptyEvent();
	}
#endif
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

		fractal = invoke_fractal(static_cast<Precision>(precision), image, drag.normalize(), fractal);
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

			fractal = invoke_fractal(static_cast<Precision>(precision), image, drag.normalize(), fractal);

			drag.x0 = drag.y0 = drag.x1 = drag.y1 = -1;
		}
	}
	if (!drag.valid()) {
		SameLine();
		Text("nothing selected");
	}

	if (Button("zoom out")) {
		if (!zoom_history.empty()) {
			Rect<fp> prev = zoom_history.back();
			fractal = invoke_fractal(
			    static_cast<Precision>(precision), image, drag.normalize(),
			    convert(prev, static_cast<Precision>(precision), static_cast<Precision>(prev.x0.index())));
			zoom_history.pop_back();
		}
	}

	if (Button("update")) {
		fractal = invoke_fractal(static_cast<Precision>(precision), image, drag, fractal);
	}

	const char *items =
#if LARGE_NUMBERS
	    "single\0double\0large\0"
#else
	    "single\0double\0"
#endif
	    ;

	int new_precision = precision;
	if (Combo("Precision", &new_precision, items)) {
		if (new_precision != precision) {
			fractal =
			    convert(fractal, static_cast<Precision>(new_precision), static_cast<Precision>(precision));
			precision = new_precision;
		}
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
	Separator();
#if 0
	Text("area");
	Text("x0 %s", fptostr(fractal.x0).c_str());
	Text("x1 %s", fptostr(fractal.x1).c_str());
	Text("y0 %s", fptostr(fractal.y0).c_str());
	Text("y1 %s", fptostr(fractal.y1).c_str());
#else
	static char buf[2048];
	snprintf(buf, sizeof(buf), "x0 %s\nx1 %s\ny0 %s\ny1 %s\n", fptostr(fractal.x0).c_str(),
	         fptostr(fractal.x1).c_str(), fptostr(fractal.y0).c_str(), fptostr(fractal.y1).c_str());
	InputTextMultiline("area", buf, strlen(buf), ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly);
#endif
	End();
}

int main(int argc, char **argv)
{
	/* {
		Fixed128 a = {0, 0};
		Fixed128 b = {0, 4};
		Fixed128 c = a - b;
		printf("%16llx %16llx\n", c.hi, c.lo);

		Fixed128 x = { 0x0001'0000'0000'0000ul , 0 };
		Fixed128 y = { 0, 4 };
		Fixed128 z = x * y;
		printf("%16llx %16llx\n", c.hi, c.lo);

	}*/

	image.width = 1280;
	image.height = 720;
	image.buf_size = image.width * image.height * 4;
	image.buf = new uint8_t[image.buf_size];

	const float ar = float(image.width) / float(image.height);
	const float scale = 0.004f;
	fractal.x0 = -image.width / 2 * scale;
	fractal.x1 = +image.width / 2 * scale;
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

	fractal = invoke_fractal(static_cast<Precision>(precision), image, Rect<int>{0, 0, image.width, image.height},
	                         fractal);

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

	calc_pool.join();

	delete[] image.buf;

	return 0;
}
