


/*
 * This file is part of [Memory Allocator].
 *
 * Copyright (C) [2026] [Maximilian Feldthusen].
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "heap.h"
#include "page.h"
#include "vmm.h"

#define MAGIC		0xB10CA7ED
#define HDRSZ		32		/* sizeof(struct blk), 16-aligned */
#define MINSPLIT	64
#define BLK_LIMIT	0x10000000000ULL
#define PG_LIMIT	0x10000000000ULL
#define PGSTACK		8192

struct blk {
	uint32_t	magic;
	uint32_t	used;
	struct blk	*next;		/* address ordered */
	struct blk	*prev;
	size_t		size;		/* payload bytes */
};

static spin_t	hl = SPIN_INIT;

static struct blk	*freelist;
static uint64_t		blkmap;		/* highest mapped va in blk region */

static uint64_t		pgtop = HPAGE_BASE;
static uint64_t		pgstk[PGSTACK];
static size_t		pgsp;

static int moreblks(uint64_t want)
{
	uint64_t npg = (want + PAGE_SIZE - 1) / PAGE_SIZE;
	uint64_t fr;
	struct blk *b, *tail;

	while (npg--) {
		fr = pmm_alloc();
		if (!fr || vmm_map(blkmap, fr, KDATA)) {
			if (fr)
				pmm_free(fr);
			return -1;
		}
		memset((void *)blkmap, 0, PAGE_SIZE);
		blkmap += PAGE_SIZE;
	}

	/* one free block over the new span, appended at tail */
	if (freelist) {
		for (tail = freelist; tail->next; tail = tail->next)
			;
		b = (struct blk *)((uint8_t *)tail + HDRSZ + tail->size);
		b->prev = tail;
		tail->next = b;
	} else {
		b = (struct blk *)HBLK_BASE;
		b->prev = NULL;
		freelist = b;
	}
	b->magic = MAGIC;
	b->used = 0;
	b->next = NULL;
	b->size = (size_t)(want + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE - HDRSZ;
	return 0;
}

void heap_init(void)
{
	blkmap = HBLK_BASE;
	pgsp = 0;
	moreblks(32 * PAGE_SIZE);
}

void *kmalloc(size_t sz)
{
	struct blk *b;
	size_t tot;

	if (!sz)
		sz = 1;
	tot = ((sz + 15) & ~15UL) + HDRSZ;

	spin_lock(&hl);
again:
	for (b = freelist; b; b = b->next) {
		if (b->used || b->size < tot)
			continue;

		if (b->size >= tot + MINSPLIT) {
			/* split off the tail */
			struct blk *r = (struct blk *)((uint8_t *)b + tot);
			r->magic = MAGIC;
			r->used = 0;
			r->size = b->size - tot;
			r->next = b->next;
			r->prev = b;
			if (r->next)
				r->next->prev = r;
			b->next = r;
			b->size = tot - HDRSZ;
		}
		b->used = 1;
		spin_unlock(&hl);
		return (uint8_t *)b + HDRSZ;
	}

	if (!moreblks(tot))
		goto again;

	spin_unlock(&hl);
	return NULL;
}

void *kzalloc(size_t sz)
{
	void *p = kmalloc(sz);

	if (p)
		memset(p, 0, sz);
	return p;
}

void kfree(void *p)
{
	struct blk *b, *n, *pv;

	if (!p)
		return;
	b = (struct blk *)((uint8_t *)p - HDRSZ);
	if (b->magic != MAGIC)
		return;

	spin_lock(&hl);
	b->used = 0;

	/* join next */
	n = b->next;
	if (n && !n->used &&
	    (uint8_t *)b + HDRSZ + b->size == (uint8_t *)n) {
		b->size += HDRSZ + n->size;
		b->next = n->next;
		if (b->next)
			b->next->prev = b;
	}
	/* join prev */
	pv = b->prev;
	if (pv && !pv->used &&
	    (uint8_t *)pv + HDRSZ + pv->size == (uint8_t *)b) {
		pv->size += HDRSZ + b->size;
		pv->next = b->next;
		if (pv->next)
			pv->next->prev = pv;
	}
	spin_unlock(&hl);
}

void *krealloc(void *p, size_t ns)
{
	struct blk *b;
	void *np;

	if (!p)
		return kmalloc(ns);
	if (!ns) {
		kfree(p);
		return NULL;
	}
	b = (struct blk *)((uint8_t *)p - HDRSZ);
	if (b->magic != MAGIC)
		return NULL;
	if (ns <= b->size)
		return p;		/* no shrink-split, don't care */

	np = kmalloc(ns);
	if (!np)
		return NULL;
	memcpy(np, p, b->size);
	kfree(p);
	return np;
}

/* whole pages for slab. freed pages get recycled via lifo, the backing
 * frames stay mapped -- returning them needs unmap + refcount. */
void *page_va_alloc(void)
{
	uint64_t fr, va;
	void *p;

	spin_lock(&hl);
	if (pgsp) {
		va = pgstk[--pgsp];
		spin_unlock(&hl);
		memset((void *)va, 0, PAGE_SIZE);
		return (void *)va;
	}
	if (pgtop >= HPAGE_BASE + PG_LIMIT) {
		spin_unlock(&hl);
		return NULL;
	}
	fr = pmm_alloc();
	if (!fr || vmm_map(pgtop, fr, KDATA)) {
		if (fr)
			pmm_free(fr);
		spin_unlock(&hl);
		return NULL;
	}
	p = (void *)pgtop;
	pgtop += PAGE_SIZE;
	spin_unlock(&hl);

	memset(p, 0, PAGE_SIZE);
	return p;
}

void page_va_free(void *va)
{
	if (!va || (uint64_t)va & (PAGE_SIZE - 1))
		return;
	spin_lock(&hl);
	if (pgsp < PGSTACK)
		pgstk[pgsp++] = (uint64_t)va;
	spin_unlock(&hl);
}

