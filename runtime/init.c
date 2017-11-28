/*
 * init.c - initializes the runtime
 */

#include <pthread.h>

#include <base/init.h>
#include <base/log.h>
#include <base/limits.h>
#include <runtime/thread.h>

#include "defs.h"

static pthread_barrier_t init_barrier;

static int runtime_init_thread(void)
{
	int ret;

	ret = base_init_thread();
	if (ret) {
		log_err("base_init_thread() failed, ret = %d", ret);
		return ret;
	}

	ret = stack_init_thread();
	if (ret) {
		log_err("stack_init_thread() failed, ret = %d", ret);
		return ret;
	}

	ret = ioqueues_init_thread();
	if (ret) {
		log_err("ioqueues_init_thread() failed, ret = %d", ret);
		return ret;
	}

	ret = sched_init_thread();
	if (ret) {
		log_err("sched_init_thread() failed, ret = %d", ret);
		return ret;
	}

	return 0;
}

static void *pthread_entry(void *data)
{
	int ret;

	ret = runtime_init_thread();
	BUG_ON(ret);

	pthread_barrier_wait(&init_barrier);
	sched_start();

	/* never reached unless things are broken */
	BUG();
	return NULL;
}

/**
 * runtime_init - starts the runtime
 * @main_fn: the first function to run as a thread
 * @arg: an argument to @main_fn
 * @threads: the number of threads to use
 *
 * Does not return if successful, otherwise return  < 0 if an error.
 */
int runtime_init(thread_fn_t main_fn, void *arg, unsigned int threads)
{
	pthread_t tid[NTHREAD];
	int ret, i;

	if (threads < 1)
		return -EINVAL;

	if (pthread_barrier_init(&init_barrier, NULL, threads) == -1) {
		log_err("pthread_barrier_init() failed, ret = %d", -errno);
		return -errno;
	}

	ret = base_init();
	if (ret) {
		log_err("base_init() failed, ret = %d", ret);
		return ret;
	}

	ret = stack_init();
	if (ret) {
		log_err("stack_init() failed, ret = %d", ret);
		return ret;
	}

	// TODO: consider deferring intialization until a socket is requested
	ret = ioqueues_init();
	if (ret) {
		log_err("ioqueues_init() failed, ret = %d", ret);
		return ret;
	}

	ret = sched_init();
	if (ret) {
		log_err("sched_init() failed, ret = %d", ret);
		return ret;
	}

	/* point of no return starts here */

	for (i = 1; i < threads; i++) {
		ret = pthread_create(&tid[i], NULL, pthread_entry, NULL);
		BUG_ON(ret);
	}

	ret = runtime_init_thread();
	BUG_ON(ret);

	ret = thread_spawn_main(main_fn, arg);
	BUG_ON(ret);

	pthread_barrier_wait(&init_barrier);
	sched_start();

	/* never reached unless things are broken */
	BUG();
	return 0;
}
