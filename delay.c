#include <stdio.h>
#include <string.h>
#include <sys/time.h>

void delay_us(unsigned long delay)
{
	struct timeval time1, time2;
	long long timel1, timel2;

	gettimeofday(&time1, 0);

	timel1 = time1.tv_sec * 1000000;
	timel1 += time1.tv_usec;

	do {
		gettimeofday(&time2, 0);
		timel2 = time2.tv_sec * 1000000;
		timel2 += time2.tv_usec;
	} while (timel2 - timel1 < delay);
}

int main()
{
	delay_us(1000000);

	return 0;
}