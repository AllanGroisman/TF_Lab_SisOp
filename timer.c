#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

volatile unsigned long sleep_time = 0;

void *timer(void *data)
{
	while (1) {
		if (sleep_time) {
			usleep(sleep_time);
			sleep_time = 0;
		} else {
			sched_yield();
		}
	}

	return 0;
}

void delay_us(unsigned long delay)
{
	sleep_time = delay;
	while (sleep_time);
}

int main()
{
	pthread_t sleeper;
	
	pthread_create(&sleeper, NULL, timer, NULL);
	
	delay_us(1000000);

	return 0;
}