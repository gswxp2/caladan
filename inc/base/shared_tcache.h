/*
 * shared_tcache.h - a generic per-thread item cache based on magazines
 */

#pragma once

#include <base/stddef.h>
#include <base/lock.h>
#include <base/list.h>
#include <base/atomic.h>
#include <base/thread.h>

#define SHARED_TCACHE_MAX_MAG_SIZE	64
#define SHARED_TCACHE_DEFAULT_MAG_SIZE	8

struct shared_tcache;

struct shared_tcache_hdr {
	struct shared_tcache_hdr	*next_item;
	struct shared_tcache_hdr	*next_mag;
};

#define SHARED_TCACHE_MIN_ITEM_SIZE	sizeof(struct shared_tcache_hdr)


struct shared_tcache_perthread {
	struct shared_tcache		*tc;
	unsigned int		rounds;
	unsigned int		capacity;
	struct shared_tcache_hdr	*loaded;
	struct shared_tcache_hdr	*previous;
};

struct shared_tcache {
	const char		*name;
	size_t			item_size;
	atomic64_t		mags_allocated;
	unsigned int		mag_size;
	spinlock_t		tc_lock;
	struct shared_tcache_hdr	*shared_mags;
	size_t 			allocated;
	size_t			capacity;
	void			**free_items;
};

extern void *__shared_tcache_alloc(struct shared_tcache_perthread *ltc);
extern void __shared_tcache_free(struct shared_tcache_perthread *ltc, void *item);

/*
 * stat counters
 */
DECLARE_PERTHREAD(uint64_t, mag_alloc);
DECLARE_PERTHREAD(uint64_t, mag_free);
DECLARE_PERTHREAD(uint64_t, pool_alloc);
DECLARE_PERTHREAD(uint64_t, pool_free);

/**
 * shared_tcache_alloc - allocates an item from the thread cache
 * @ltc: the thread-local cache
 *
 * Returns an item, or NULL if out of memory.
 */
static inline void *shared_tcache_alloc(struct shared_tcache_perthread *ltc)
{
	void *item = (void *)ltc->loaded;

	if (ltc->rounds == 0)
		return __shared_tcache_alloc(ltc);

	ltc->rounds--;
	ltc->loaded = ltc->loaded->next_item;
	return item;
}

/**
 * shared_tcache_free - frees an item to the thread cache
 * @ltc: the thread-local cache
 * @item: the item to free
 */
static inline void shared_tcache_free(struct shared_tcache_perthread *ltc, void *item)
{
	struct shared_tcache_hdr *hdr = (struct shared_tcache_hdr *)item;

	if (ltc->rounds >= ltc->capacity)
		return __shared_tcache_free(ltc, item);

	ltc->rounds++;
	hdr->next_item = ltc->loaded;
	ltc->loaded = hdr;
}

extern struct shared_tcache *shared_tcache_create(const char *name,
			     unsigned int mag_size, size_t item_size,
				 void *buf, size_t len,
		   		 size_t pgsize, size_t item_len);
extern void shared_tcache_init_perthread(struct shared_tcache *tc,
				  struct shared_tcache_perthread *ltc);
extern void shared_tcache_reclaim(struct shared_tcache *tc);
extern void shared_tcache_print_usage(void);
