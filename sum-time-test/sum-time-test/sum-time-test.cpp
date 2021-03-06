/*
 Copyright (c) 2022 Howard Wu
 Licensed under the MIT Licence
*/

#include <iostream>
#include <iomanip>
#include <thread>
#include <climits>
#include "../../Thread-Pool-Cpp/ThreadPool.hpp"

using namespace std;

constexpr auto MAX_NUM = 6000000000;

int unsigned_int_overflow(unsigned int var_a, unsigned int var_b)
{
	if (UINT_MAX - var_a < var_b)
	{
		std::cout << "warn: unsigned_int_overflow!" << std::endl;
		return 1;
	}
	else
		return 0;
}
int unsigned_llong_overflow(unsigned long long var_a, unsigned long long var_b)
{
	if (ULLONG_MAX - var_a < var_b)
	{
		std::cout << "warn: unsigned_llong_overflow!" << std::endl;
		return 1;
	}
	else
		return 0;
}
//性能优化，没用到
void sumUpTasks(std::promise<unsigned long long>&& prom, const unsigned long long start, const unsigned long long end)
{
	unsigned long long sum = {};
	for (auto i = start; i <= end; i++)
	{
#ifdef _DEBUG
		if (unsigned_llong_overflow(sum, temp) == 1)
			return;
#endif // _DEBUG
		sum += i;
	}
	prom.set_value(sum);
}
thread_local unsigned long long tmpSum = 0;//thread_local指定仅在当前填充的线程有效，对于其他线程是隔离的
void sumUp(std::atomic<unsigned long long>& sum, const unsigned long long start, const unsigned long long end)
{//atomic原子操作 lock free
	for (auto i = start; i <= end; i++)
	{
#ifdef _DEBUG
		if (unsigned_llong_overflow(tmpSum, temp) == 1)
			return;
#endif // _DEBUG
		tmpSum += i;
	}
	sum.fetch_add(tmpSum, std::memory_order_relaxed);//atomic的fetch_add lock free
}
int main(int argc, char* argv[])
{
	unsigned long long count_to = MAX_NUM;
	unsigned short thread_count = 8;
	std::atomic<unsigned long long> sum{};
	std::cout << "Time Test of Single-threaded and Multi-threaded summation" << std::endl;

#ifdef _DEBUG
	std::cout << "DEBUG builds may cause performance degradation!" << std::endl
		<< std::endl;
#endif // _DEBUG

	std::cout << std::setiosflags(std::ios::fixed);

	if (argc >= 3)
	{
		thread_count = atoi(argv[1]);
		auto input_num = strtoull(argv[2], NULL, 10);
		if (input_num <= MAX_NUM)
			count_to = input_num;
	}
	std::cout << "Count to " << count_to << std::endl;

	// Single-threaded

	auto start_time = std::chrono::steady_clock::now();

	//性能优化，没用到
	//std::promise<unsigned long long> prom1;
	//auto fut1 = prom1.get_future();
	////std::thread t1(sumUpTasks, std::move(prom1), 1, count_to);
	//sumUpTasks(std::move(prom1), 1, count_to);
	//sum = fut1.get();

	std::thread t1(sumUp, std::ref(sum), 0, count_to);//公平起见，用与多线程相同的方法
	t1.join();

	auto diff = std::chrono::steady_clock::now() - start_time;
	std::cout << "Single-threaded:" << std::endl
		<< "time=" << std::chrono::duration<double, std::milli>(diff).count() << "ms" << std::endl
		<< "sum=" << sum << std::endl;
	// End Single-threaded

	// Multi-threaded
	std::cout << "Multi-threaded:" << std::endl;
	sum = 0;
	unsigned long long start;
	unsigned long long end;
	auto block_size = count_to / thread_count;
	thread_pool pool{ thread_count };//线程池 数量thread_count
	cout << "threads=" << pool.thread_count() << std::endl;
	start_time = std::chrono::steady_clock::now();
	for (auto i = 0; i < thread_count; i++)
	{
		start = i != 0 ? i * block_size + 1 : i * block_size;
		end = (static_cast<unsigned long long>(i) + 1) * block_size;
		pool.push(std::bind(sumUp, std::ref(sum), start, end));//往池中加任务
	}

	pool.join();//池中所有进程join，计时才有意义
	diff = std::chrono::steady_clock::now() - start_time;

	cout << "time=" << std::chrono::duration<double, std::milli>(diff).count() << "ms" << std::endl
		<< "sum=" << sum << std::endl;
	// End Multi-threaded
}
