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

struct ticket_mutex
{
private:
	alignas(std::hardware_constructive_interference_size) std::atomic<unsigned> in = ATOMIC_VAR_INIT(0);
	alignas(std::hardware_constructive_interference_size) std::atomic<unsigned> out = ATOMIC_VAR_INIT(0);
public:
	void lock()
	{
		auto const my = in.fetch_add(1, std::memory_order_acquire);
		while (true)
		{
			auto const now = out.load(std::memory_order_acquire);
			if (now == my)
				return;
			out.wait(now, std::memory_order_relaxed);
		}
	}

	void unlock()
	{
		out.fetch_add(1, std::memory_order_release);
		out.notify_all();
	}
};

//#define USE_LIGHT_MUTEX
//#define USE_TICKET_MUTEX

#ifdef USE_LIGHT_MUTEX
using Mutex = light_mutex;
#elif defined USE_TICKET_MUTEX
using Mutex = ticket_mutex;
#else
using Mutex = std::mutex;
#endif

Mutex mutex;

int value = 0;
constexpr unsigned THREADS = 10;

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
				for (auto Try : std::ranges::views::iota(0, 10'000))
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