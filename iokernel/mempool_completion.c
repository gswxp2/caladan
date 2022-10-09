/*
 * mempool_completion.c - a single producer, single consumer mempool that sends
 * completion events when tx buffers can be freed. Based on rte_mempool_stack.c.
 */

#include <rte_mempool.h>
#include <rte_malloc.h>

#include <base/log.h>

#include "defs.h"

//#define COMPLETE_USE_SPINLOCK 0

struct completion_stack {
	uint32_t size;
	uint32_t len;
	#ifdef COMPLETE_USE_SPINLOCK
		spinlock_t lock;
	#endif
	void *objs[];
};

static int completion_enqueue(struct rte_mempool *mp, void * const *obj_table,
		unsigned n)
{
	unsigned long i;
	struct completion_stack *s = mp->pool_data;
	#ifdef COMPLETE_USE_SPINLOCK
	spin_lock(&s->lock);
	#endif
	if (unlikely(s->len + n > s->size))
		return -ENOBUFS;

	for (i = 0; i < n; i++)
		// Give up on notifying the runtime if this returns false.
		tx_send_completion(obj_table[i]);

#pragma GCC ivdep
	for (i = 0; i < n; i++)
		s->objs[s->len + i] = obj_table[i];

	s->len += n;
	#ifdef COMPLETE_USE_SPINLOCK
	spin_unlock(&s->lock);
	#endif

	return 0;
}

static int completion_dequeue(struct rte_mempool *mp, void  ** obj_table, unsigned n)
{
	unsigned long i, j;
	struct completion_stack *s = mp->pool_data;
	#ifdef COMPLETE_USE_SPINLOCK
	spin_lock(&s->lock);
	#endif
	if (unlikely(n > s->len)){
		#ifdef COMPLETE_USE_SPINLOCK
		spin_unlock(&s->lock);
		#endif
		return -ENOBUFS;
	}

	s->len -= n;
#pragma GCC ivdep
	for (i = 0, j = s->len; i < n; i++, j++)
		obj_table[i] = s->objs[j];
	
	#ifdef COMPLETE_USE_SPINLOCK
	spin_unlock(&s->lock);
	#endif
	return 0;
}

static unsigned completion_get_count(const struct rte_mempool *mp)
{
	struct completion_stack *s = mp->pool_data;
	return s->len;
}

static int completion_alloc(struct rte_mempool *mp)
{
	struct completion_stack *s;
	unsigned n = mp->size;
	int size = sizeof(*s) + (n + 16) * sizeof(void *);
	s = rte_zmalloc_socket(mp->name, size, RTE_CACHE_LINE_SIZE, mp->socket_id);
	if (!s) {
		log_err("Could not allocate stack");
		return -ENOMEM;
	}

	s->len = 0;
	s->size = n;
	mp->pool_data = s;
	#ifdef COMPLETE_USE_SPINLOCK
	s->lock.locked=0;
	#endif
	return 0;
}

static void completion_free(struct rte_mempool *mp)
{
	rte_free(mp->pool_data);
}

/*
 * Dummy mempool that sends completion events on enqueue and does nothing else.
 */
static const struct rte_mempool_ops ops_completion = {
	.name = "completion",
	.alloc = completion_alloc,
	.free = completion_free,
	.enqueue = completion_enqueue,
	.dequeue = completion_dequeue,
	.get_count = completion_get_count,
};

MEMPOOL_REGISTER_OPS(ops_completion);
