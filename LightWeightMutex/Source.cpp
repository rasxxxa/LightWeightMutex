#include <iostream>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <ranges>
#include <numeric>
#include <algorithm>
#include <chrono>

struct light_mutex
{
private:
	std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
	void lock()
	{
		while (flag.test_and_set(std::memory_order_acquire))
			flag.wait(true, std::memory_order_relaxed);
	}

	void unlock()
	{
		flag.clear(std::memory_order_release);
		flag.notify_one();
	}
};

//#define USE_CUSTOM_MUTEX
#ifdef USE_CUSTOM_MUTEX
using Mutex = light_mutex;
#else
using Mutex = std::mutex;
#endif

Mutex mutex;

int value = 0;
constexpr unsigned THREADS = 100;

void Produce()
{
	mutex.lock();
	value++;
	mutex.unlock();
}

int main()
{
	std::vector<std::jthread> threads;
	threads.resize(THREADS);
	auto startPoint = std::chrono::high_resolution_clock::now();

	for (auto thread : std::ranges::views::iota(unsigned(0), THREADS))
	{
//		std::cout << "Thread " << thread << " started!" << std::endl;
		threads[thread] = std::jthread([]() 
			{
				for (auto Try : std::ranges::views::iota(0, 10'000'000))
					Produce();
			});
	}

	std::ranges::for_each(threads, [](std::jthread& thread) {thread.join(); });

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	auto endPoint = std::chrono::high_resolution_clock::now();
	auto diff = (endPoint - startPoint).count() / 1'000'000;

	std::cout << "Result is: " << value << std::endl;
	std::cout << "Time passed " << diff << " milliseconds!" << std::endl;

}