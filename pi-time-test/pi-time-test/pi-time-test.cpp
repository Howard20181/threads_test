/*
 Copyright (c) 2022 Howard Wu
 Licensed under the MIT Licence
*/

#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include <thread>
#include <climits>
#include <cmath>
#include "../../Thread-Pool-Cpp/ThreadPool.hpp"


using namespace std;

void sumUp(std::atomic<double>& sum, const double start, const double end)
{
	double factor = fmod(start, 2) == 0 ? 1.0 : -1.0;
#ifdef _DEBUG
	cout << "start=" << start << " end=" << end << " factor=" << factor << endl;
#endif // _DEBUG

	double tmpSum = 0.0;
	for (auto i = start; i <= end; i++, factor = -factor) {
		tmpSum += factor / (2 * i + 1);
	}
	sum = sum + tmpSum;
}
int main(int argc, char* argv[])
{
	std::chrono::steady_clock::time_point start_time;
	std::chrono::duration<long long, std::nano> diff;
	unsigned long long n = 1000000000;
	unsigned short thread_count = 4;
	std::atomic<double> sum{};
	double pi = 0.0;
	std::cout << "Time Test of Single-threaded and Multi-threaded pi-value solution" << std::endl;

#ifdef _DEBUG
	std::cout << "DEBUG builds may cause performance degradation!" << std::endl
		<< std::endl;
#endif // _DEBUG

	std::cout << std::setiosflags(std::ios::fixed) << setprecision(15);

	if (argc >= 3)
	{
		thread_count = atoi(argv[1]);
		auto input_num = strtoull(argv[2], NULL, 10);
		n = input_num;
	}
	bool isSkipSingle = argc >= 4 && atoi(argv[3]) == 1;

	std::cout << "n=" << n << std::endl;
	if (isSkipSingle)
	{
		cout << "Skip Single-thread Test!" << endl;
		goto Multi;
	}
	// Single-threaded

	start_time = std::chrono::steady_clock::now();

	if (!isSkipSingle)
	{
		std::thread t1(sumUp, std::ref(sum), 0, static_cast<double>(n));
		t1.join();
	}
	pi = 4.0 * sum;
	diff = std::chrono::steady_clock::now() - start_time;
	std::cout << "Single-threaded:" << std::endl
		<< "time=" << std::chrono::duration<double, std::milli>(diff).count() << "ms" << std::endl
		<< "C_PI=" << pi << std::endl
		<< "M_PI=" << M_PI << std::endl;
	// End Single-threaded
Multi:
	// Multi-threaded
	std::cout << "Multi-threaded:" << std::endl;
	sum = 0;
	double start;
	double end;
	double block_size = static_cast<double>(n) / thread_count;
	thread_pool pool{ thread_count };
	cout << "threads=" << pool.thread_count() << std::endl;
	start_time = std::chrono::steady_clock::now();
	for (auto i = 0; i < thread_count; i++)
	{
		start = i != 0 ? i * block_size + 1 : i * block_size;
		end = (i + 1) * block_size;
		pool.push(std::bind(sumUp, std::ref(sum), start, end));
	}

	pool.join();
	pi = 4.0 * sum;
	diff = std::chrono::steady_clock::now() - start_time;

	cout << "time=" << std::chrono::duration<double, std::milli>(diff).count() << "ms" << std::endl
		<< "C_PI=" << pi << std::endl
		<< "M_PI=" << M_PI << std::endl;
	// End Multi-threaded

	cout << "end" << endl;
}
