/*
 Copyright (c) 2022 Howard Wu
 Licensed under the MIT Licence
*/

#include <iostream>
#include <iomanip>
#include <thread>
#include <climits>
#include <pthread.h>

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
struct sumUpInput
{
	unsigned long long start;
	unsigned long long end;
};
pthread_mutex_t mutexsum;
unsigned long long sum;
void *sumUp(void *arg)
{
	unsigned long long tmpSum = 0; // thread_local指定仅在当前填充的线程有效，对于其他线程是隔离的
	sumUpInput input = *(sumUpInput *)arg;
	for (auto i = input.start; i <= input.end; i++)
	{
#ifdef _DEBUG
		if (unsigned_llong_overflow(tmpSum, temp) == 1)
			return;
#endif // _DEBUG
		tmpSum += i;
	}
	pthread_mutex_lock(&mutexsum);
	sum += tmpSum;
	pthread_mutex_unlock(&mutexsum);
}
int main(int argc, char *argv[])
{
	unsigned long long count_to = MAX_NUM;
	unsigned short thread_count = 8;
	std::cout << "Time Test of Single-threaded and Multi-threaded summation" << std::endl;
	sumUpInput input;
#ifdef _DEBUG
	std::cout << "DEBUG builds may casumUpuse performance degradation!" << std::endl
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
	input.start = 0;
	input.end = count_to;
	sumUp(&input);

	auto diff = std::chrono::steady_clock::now() - start_time;
	std::cout << "Single-threaded:" << std::endl
			  << "time=" << std::chrono::duration<double, std::milli>(diff).count() << "ms" << std::endl
			  << "sum=" << sum << std::endl;
	// End Single-threaded

	// Multi-threaded
	sum = 0;
	std::cout << "Multi-threaded:" << std::endl;
	pthread_t threads[thread_count];
	pthread_attr_t attr;
	int rc;
	auto block_size = count_to / thread_count;

	cout << "threads=" << thread_count << std::endl;
	pthread_attr_init(&attr);
	pthread_mutex_init(&mutexsum, NULL);
	start_time = std::chrono::steady_clock::now();
	for (auto i = 0; i < thread_count; i++)
	{
		input.start = i != 0 ? i * block_size + 1 : i * block_size;
		input.end = (static_cast<unsigned long long>(i) + 1) * block_size;
		rc = pthread_create(&threads[i], &attr, sumUp, &input);
	}
	for (auto i = 0; i < thread_count; i++)
	{
		rc = pthread_join(threads[i], NULL);
		if (rc)
		{
			cout << "ERROR; return code from pthread_join() is " << rc << endl;
			exit(-1);
		}
	}
	diff = std::chrono::steady_clock::now() - start_time;
	pthread_mutex_destroy(&mutexsum);
	cout << "time=" << std::chrono::duration<double, std::milli>(diff).count() << "ms" << std::endl
		 << "sum=" << sum << std::endl;
	// End Multi-threaded
}
