/*
 * shared_tcache.c - a generic thread-local item cache
 *
 * Based heavily on Magazines and Vmem: Extending the Slab Allocator to Many
 * CPUs and Arbitrary Resources. Jeff Bonwick and Johnathan Adams.
 *
 * TODO: Improve NUMA awareness.
 * TODO: Provide an interface to tear-down thread caches.
 * TODO: Remove dependence on libc malloc().
 * TODO: Use RCU for shared_tcache list so printing stats doesn't block creating
 * new shared_tcaches.
 */

#include <stdlib.h>

#include <base/stddef.h>
#include <base/log.h>
#include <base/lock.h>
#include <base/shared_tcache.h>
#include <base/thread.h>

// DEFINE_PERTHREAD(uint64_t, mag_alloc);
// DEFINE_PERTHREAD(uint64_t, mag_free);
// DEFINE_PERTHREAD(uint64_t, pool_alloc);
// DEFINE_PERTHREAD(uint64_t, pool_free);

static inline void *mempool_alloc(struct shared_tcache *m)
{
	void *item;
	if (unlikely(m->allocated >= m->capacity))
		return NULL;
	item = m->free_items[m->allocated++];
	// __mempool_alloc_debug_check(m, item);
	return item;
}

/**
 * mempool_free - returns an item to the pool
 * @m: the memory pool the item was allocated from
 * @item: the item to return
 */
static inline void mempool_free(struct shared_tcache *m, void *item)
{
	// __mempool_free_debug_check(m, item);
	m->free_items[--m->allocated] = item;
	assert(m->allocated <= m->capacity); /* could have overflowed */
}

/**
 * shared_tcache_top_free - returns some items to the pool
 * must hold the shared_tcache_lock
 * @tc: the shared_tcache the items were allocated from
 * @nr: the number of items to return
 * @item: the items to return
 */
static void shared_tcache_top_free(struct shared_tcache *tc, int nr, void **items)
{
	int i;
	for (i = 0; i < nr; i++) {
		mempool_free(tc, items[i]);
	}
}

static int shared_tcache_top_alloc(struct shared_tcache *tc, int nr, void **items)
{
	int i;
	for (i = 0; i < nr; i++) {
		items[i] = mempool_alloc(tc);
		if (items[i] == NULL) {
			shared_tcache_top_free(tc, i, items);
			return -ENOMEM;
		}
	}
	return 0;
}

static struct shared_tcache_hdr *shared_tcache_alloc_mag(struct shared_tcache *tc)
{
	void *items[SHARED_TCACHE_MAX_MAG_SIZE];
	struct shared_tcache_hdr *head, **pos;
	int err, i;

	perthread_get(mag_alloc)++;
	spin_lock(&tc->tc_lock);
	err = shared_tcache_top_alloc(tc, tc->mag_size, items);
	spin_unlock(&tc->tc_lock);

	if (err)
		return NULL;

	head = (struct shared_tcache_hdr *)items[0];
	pos = &head->next_item;
	for (i = 1; i < tc->mag_size; i++) {
		*pos = (struct shared_tcache_hdr *)items[i];
		pos = &(*pos)->next_item;
	}

	*pos = NULL;
	atomic64_inc(&tc->mags_allocated);
	return head;
}

static void shared_tcache_free_mag(struct shared_tcache *tc, struct shared_tcache_hdr *hdr)
{
	void *items[SHARED_TCACHE_MAX_MAG_SIZE];
	int nr = 0;

	perthread_get(mag_free)++;

	do {
		items[nr++] = hdr;
		hdr = hdr->next_item;
	} while (hdr);

	assert(nr == tc->mag_size);
	spin_lock(&tc->tc_lock);
	shared_tcache_top_free(tc, nr, items);
	spin_unlock(&tc->tc_lock);
	atomic64_dec(&tc->mags_allocated);
}

/* The thread-local cache allocation slow path. */
void *__shared_tcache_alloc(struct shared_tcache_perthread *ltc)
{
	struct shared_tcache *tc = ltc->tc;
	void *item;

	/* must be out of rounds */
	assert(ltc->rounds == 0);
	assert(ltc->loaded == NULL);

	/* CASE 1: exchange empty loaded mag with full previous mag */
	if (ltc->previous) {
		ltc->loaded = ltc->previous;
		ltc->previous = NULL;
		goto alloc;
	}

	perthread_get(pool_alloc)++;

	/* CASE 2: grab a magazine from the shared pool */
	spin_lock(&tc->tc_lock);
	ltc->loaded = tc->shared_mags;
	if (tc->shared_mags)
		tc->shared_mags = tc->shared_mags->next_mag;
	spin_unlock(&tc->tc_lock);
	if (ltc->loaded)
		goto alloc;

	/* CASE 3: allocate a new magazine */
	// printf("shared_tcache alloc slow slow path %p\n",ltc);
	ltc->loaded = shared_tcache_alloc_mag(tc);
	if (unlikely(!ltc->loaded)){
		//spin_unlock(&ltc->lock);
		return NULL;
	}

alloc:
	/* reload the magazine and allocate an item */
	ltc->rounds = ltc->capacity - 1;
	item = (void *)ltc->loaded;
	ltc->loaded = ltc->loaded->next_item;
	//spin_unlock(&ltc->lock);
	return item;
}

/* The thread-local cache free slow path. */
void __shared_tcache_free(struct shared_tcache_perthread *ltc, void *item)
{
	struct shared_tcache *tc = ltc->tc;
	struct shared_tcache_hdr *hdr = (struct shared_tcache_hdr *)item;

	/* magazine must be full */
	__build_assert_if_constant(ltc->rounds == ltc->capacity);
	assert(ltc->loaded != NULL);

	/* CASE 1: exchange empty previous mag with full loaded mag */
	if (!ltc->previous) {
		ltc->previous = ltc->loaded;
		goto free;
	}

	perthread_get(pool_free)++;

	/* CASE 2: return a magazine to the shared pool */
	spin_lock(&tc->tc_lock);
	ltc->previous->next_mag = tc->shared_mags;
	tc->shared_mags = ltc->previous;
	spin_unlock(&tc->tc_lock);
	ltc->previous = ltc->loaded;

free:
	/* start a new magazine and free the item */
	ltc->rounds = 1;
	ltc->loaded = hdr;
	hdr->next_item = NULL;
	//spin_unlock(&ltc->lock);
}

/**
 * shared_tcache_create - creates a new thread-local cache
 * @name: a human-readable name to identify the cache
 * @ops: operations for allocating and freeing items that back the cache
 * @mag_size: the number of items in a magazine
 * @item_size: the size of each item
 *
 * Returns a thread cache or NULL of out of memory.
 *
 * After creating a thread-local cache, you'll want to attach one or more
 * thread-local handles using shared_tcache_init_perthread().
 */
struct shared_tcache *shared_tcache_create(const char *name,
			     unsigned int mag_size, size_t item_size,
				 void *buf, size_t len,
		   		 size_t pgsize, size_t item_len)
{
	struct shared_tcache *tc;

	/* we assume the caller is aware of the shared_tcache size limits */
	assert(item_size >= SHARED_TCACHE_MIN_ITEM_SIZE);
	assert(mag_size <= SHARED_TCACHE_MAX_MAG_SIZE);

	tc = buf;
	if (!tc)
		return NULL;

	tc->name = name;
	tc->item_size = item_size;
	atomic64_write(&tc->mags_allocated, 0);
	tc->mag_size = mag_size;
	spin_lock_init(&tc->tc_lock);
	tc->shared_mags = NULL;
	tc->allocated = 0;
	tc->capacity = 0;
	tc->free_items= buf+sizeof(struct shared_tcache);
	size_t items_per_page = pgsize / item_len;
	size_t nr_pages = len / pgsize-1;
	assert (nr_pages*items_per_page*sizeof(void*) <= pgsize-sizeof(struct shared_tcache));
	for (int i = 0; i < nr_pages; i++) {
		for (int j = 0; j < items_per_page; j++) {
			tc->free_items[tc->capacity++] =
				(char *)buf +pgsize+pgsize * i + item_len * j;
		}
	}
	return tc;
}

/**
 * shared_tcache_init_perthread - intializes a per-thread handle for a thread-local
 *                         cache
 * @tc: the thread-local cache
 * @ltc: the per-thread handle
 */
void shared_tcache_init_perthread(struct shared_tcache *tc, struct shared_tcache_perthread *ltc)
{
	ltc->tc = tc;
	ltc->loaded = ltc->previous = NULL;
	ltc->rounds = 0;
	ltc->capacity = tc->mag_size; 
}

/**
 * shared_tcache_reclaim - reclaims unused memory from a thread-local cache
 * @tc: the thread-local cache
 */
void shared_tcache_reclaim(struct shared_tcache *tc)
{
	struct shared_tcache_hdr *hdr, *next;

	spin_lock(&tc->tc_lock);
	hdr = tc->shared_mags;
	tc->shared_mags = NULL;
	spin_unlock(&tc->tc_lock);

	while (hdr) {
		next = hdr->next_mag;
		shared_tcache_free_mag(tc, hdr);
		hdr = next;
	}
}

// /**
//  * shared_tcache_print_stats - dumps usage statistics about all thread-local caches
//  */
// void shared_tcache_print_usage(void)
// {
// 	struct shared_tcache *tc;
// 	size_t total = 0;

// 	log_info("shared_tcache: dumping usage statistics...");

// 	spin_lock(&shared_tcache_lock);
// 	list_for_each(&shared_tcache_list, tc, link) {
// 		long mags = atomic64_read(&tc->mags_allocated);
// 		size_t usage = tc->mag_size * tc->item_size * mags;
// 		log_info("%8ld KB\t%s", usage / 1024, tc->name);
// 		total += usage;
// 	}
// 	spin_unlock(&shared_tcache_lock);

// 	log_info("total: %8ld KB", total / 1024);
// }
