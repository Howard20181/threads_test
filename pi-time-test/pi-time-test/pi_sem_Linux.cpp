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
#include <pthread.h>
#include <semaphore.h>

using namespace std;
struct sumUpInput
{
	double start;
	double end;
};
sem_t mutexsum;
double sum;
void *sumUp(void *arg)
{
	auto input = (sumUpInput *)arg;
	double factor = fmod(input->start, 2) == 0 ? 1.0 : -1.0;

	double tmpSum = 0.0;
	for (auto i = input->start; i <= input->end; i++, factor = -factor)
	{
		tmpSum += factor / (2 * i + 1);
	}
	sem_wait(&mutexsum);
	sum += tmpSum;
	sem_post(&mutexsum);
	return NULL;
}
int main(int argc, char *argv[])
{
	chrono::steady_clock::time_point start_time;
	chrono::duration<long long, nano> diff;
	unsigned long long n = 1000000000;
	unsigned short thread_count = 4;
	double pi = 0.0;
	cout << "Multi-threaded pi-value solution" << endl;
	sumUpInput input;
#ifdef _DEBUG
	cout << "DEBUG builds may cause performance degradation!" << endl
		 << endl;
#endif // _DEBUG

	cout << setiosflags(ios::fixed) << setprecision(15);

	if (argc >= 3)
	{
		thread_count = atoi(argv[1]);
		auto input_num = strtoull(argv[2], NULL, 10);
		n = input_num;
	}
	bool isSkipSingle = argc >= 4 && atoi(argv[3]) == 1;

	cout << "n=" << n << endl;

Multi:
	// Multi-threaded
	cout << "Multi-threaded:" << endl;
	pthread_t threads[thread_count];
	pthread_attr_t attr;
	int rc;
	double block_size = static_cast<double>(n) / thread_count;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	sem_init(&mutexsum, 0, 1);
	cout << "threads=" << thread_count << endl;
	start_time = chrono::steady_clock::now();
	for (auto i = 0; i < thread_count; i++)
	{
		input.start = i != 0 ? i * block_size + 1 : i * block_size;
		input.end = (i + 1) * block_size;
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
	pi = 4.0 * sum;
	diff = chrono::steady_clock::now() - start_time;
	sem_destroy(&mutexsum);
	cout << "time=" << chrono::duration<double, milli>(diff).count() << "ms" << endl
		 << "C_PI=" << pi << endl
		 << "M_PI=" << M_PI << endl;
	// End Multi-threaded

	cout << "end" << endl;
}
