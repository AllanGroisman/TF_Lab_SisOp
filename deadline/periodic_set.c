#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#define SCHED_DEADLINE       6

/* XXX use the proper syscall numbers */
#ifdef __x86_64__
#define __NR_sched_setattr           314
#define __NR_sched_getattr           315
#endif

#ifdef __i386__
#define __NR_sched_setattr           351
#define __NR_sched_getattr           352
#endif

#ifdef __arm__
#define __NR_sched_setattr           380
#define __NR_sched_getattr           381
#endif

struct sched_attr {
	__u32 size;

	__u32 sched_policy;
	__u64 sched_flags;

	/* SCHED_NORMAL, SCHED_BATCH */
	__s32 sched_nice;

	/* SCHED_FIFO, SCHED_RR */
	__u32 sched_priority;

	/* SCHED_DEADLINE (nsec) */
	__u64 sched_runtime;
	__u64 sched_deadline;
	__u64 sched_period;
};

int sched_setattr(pid_t pid,
               const struct sched_attr *attr,
               unsigned int flags)
{
	return syscall(__NR_sched_setattr, pid, attr, flags);
}

volatile int done = 0;

void *task1(void *data)
{
	int ret;
	int flags = 0;
	struct sched_attr attr;

	memset(&attr, 0, sizeof(attr));
	attr.size = sizeof(attr);
	/* This creates a 10ms / 50ms reservation */
	attr.sched_policy = SCHED_DEADLINE;
	attr.sched_runtime = 10000000;
	attr.sched_deadline = attr.sched_period = 50000000;
	ret = sched_setattr(0, &attr, flags);
	if (ret < 0) {
		perror("sched_setattr failed to set the priorities");
		exit(-1);
	}

	/* doing something important */
	while (!done) {
	}
	
	return 0;
}

void *task2(void *data)
{
	int ret;
	int flags = 0;
	struct sched_attr attr;

	memset(&attr, 0, sizeof(attr));
	attr.size = sizeof(attr);
	/* This creates a 10ms / 60ms reservation */
	attr.sched_policy = SCHED_DEADLINE;
	attr.sched_runtime = 10000000;
	attr.sched_deadline = attr.sched_period = 60000000;
	ret = sched_setattr(0, &attr, flags);
	if (ret < 0) {
		perror("sched_setattr failed to set the priorities");
		exit(-1);
	}

	/* doing something important */
	while (!done) {
	}
	
	return 0;
}

void *task3(void *data)
{
	int ret;
	int flags = 0;
	struct sched_attr attr;

	memset(&attr, 0, sizeof(attr));
	attr.size = sizeof(attr);
	/* This creates a 20ms / 80ms reservation */
	attr.sched_policy = SCHED_DEADLINE;
	attr.sched_runtime = 30000000;
	attr.sched_deadline = attr.sched_period = 80000000;
	ret = sched_setattr(0, &attr, flags);
	if (ret < 0) {
		perror("sched_setattr failed to set the priorities");
		exit(-1);
	}

	/* doing something important */
	while (!done) {
	}
	
	return 0;
}

int main(int argc, char **argv)
{
	pthread_t threads[3];

	/* create three threads with the following parameters:
	 * t1: period = 50ms, wcet = 10ms, deadline = 50ms
	 * t2: period = 60ms, wcet = 10ms, deadline = 60ms
	 * t3: period = 80ms, wcet = 20ms, deadline = 80ms
	 * 
	 * utilization factor: ~61%
	 */
	pthread_create(&threads[0], NULL, task1, NULL);
	pthread_create(&threads[1], NULL, task2, NULL);
	pthread_create(&threads[2], NULL, task3, NULL);

	sleep(10);

	done = 1;
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);

	return 0;
}
