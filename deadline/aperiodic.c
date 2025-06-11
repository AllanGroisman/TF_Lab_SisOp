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

int main(int argc, char **argv)
{
	int ret;
	int flags = 0;
	struct sched_attr attr;
	volatile long long count = 0;

	memset(&attr, 0, sizeof(attr));
	attr.size = sizeof(attr);
	/* This creates a 200ms / 1s reservation */
	attr.sched_policy = SCHED_DEADLINE;
	attr.sched_runtime = 200000000;
	attr.sched_deadline = attr.sched_period = 1000000000;
	ret = sched_setattr(0, &attr, flags);
	if (ret < 0) {
		perror("sched_setattr failed to set the priorities");
		exit(-1);
	}

	/* doing something important, but once in a while */
	while (count++ < 500000000);

	exit(0);
}
