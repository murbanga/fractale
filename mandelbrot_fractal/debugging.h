#pragma once
#ifdef WIN32
#include <windows.h>
#endif

#include <atomic>

struct progress_info
{
	std::atomic<int> progress = 0;
	double execution_time_sec = 0;
};

class Profiler
{
public:
	Profiler()
	{
#ifdef WIN32
		LARGE_INTEGER i;
		QueryPerformanceCounter(&i);
		beg = i.QuadPart;
#else
		timeval tv;
		gettimeofday(&tv, nullptr);
		beg = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
#endif
	}

	double elapsed_time() const
	{
#ifdef WIN32
		LARGE_INTEGER end, freq;
		QueryPerformanceCounter(&end);
		QueryPerformanceFrequency(&freq);
		return double(end.QuadPart - beg) / double(freq.QuadPart);
#else
		timeval tv;
		gettimeofday(&tv, nullptr);
		uint64_t end = tv.tv_sec * 1000 * 1000 + tv.tv_usec;
		return double(end - beg) / double(1000 * 1000);
#endif
	}

	double get() const
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
};