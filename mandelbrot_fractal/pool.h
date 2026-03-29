#pragma once
#include <functional>
#include <thread>

class pool {
	public:
	pool() : finished(0) {}

	pool(std::function<void(int)> g) : finished(0) {}

	~pool() { join(); }

	void start(int nthreads, int iterations, std::function<void(int)> f)
	{
		cancel = false;
		finished = 0;
		for (int i = 0; i < nthreads; ++i)
			t.push_back(std::thread([i, iterations, f, this]() {
				for (int j = 0; !cancel && j < iterations; ++j)
					f(i * iterations + j);
				finished++;
			}));
	}

	void join()
	{
		cancel = true;
		for (int i = 0; i < t.size(); ++i)
			if (t[i].joinable())
				t[i].join();

		t.clear();
		finished = 0;
	}

	bool is_finished() const { return finished == t.size(); }
	bool empty() const { return t.empty(); }

	private:
	bool cancel;
	std::atomic<int> finished;
	std::vector<std::thread> t;
};
