#pragma once
#ifdef WIN32
#include <windows.h>
#endif

#include <atomic>

struct progress_info
{
	std::atomic<int> progress_num = 0;
	int progress_den = 0;
	double execution_time_sec = 0;
};

#define PROF	Profiler _prof(__FUNCTION__)

class Profiler
{
public:
	Profiler()
	{
		start();
	}

	~Profiler()
	{
		double t = elapsed_time();
	}

	void start()
	{
#ifdef WIN32
		LARGE_INTEGER i;
		QueryPerformanceCounter(&i);
		beg = i.QuadPart;
		QueryPerformanceFrequency(&freq);
#else
		timeval tv;
		gettimeofday(&tv, nullptr);
		beg = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
#endif
	}

	double elapsed_time() const
	{
#ifdef WIN32
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);
		return double(end.QuadPart - beg) / double(freq.QuadPart);
#else
		timeval tv;
		gettimeofday(&tv, nullptr);
		uint64_t end = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
		return double(end - beg) / double(1000 * 1000);
#endif
	}

	static double get()
	{
#ifdef WIN32
		LARGE_INTEGER end, freq;
		QueryPerformanceCounter(&end);
		QueryPerformanceFrequency(&freq);
		return double(end.QuadPart) / double(freq.QuadPart);
#else
		timeval tv;
		gettimeofday(&tv, nullptr);
		uint64_t end = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
		return double(end) / double(1000 * 1000);
#endif
	}
private:
	uint64_t beg;
	LARGE_INTEGER freq;
};
