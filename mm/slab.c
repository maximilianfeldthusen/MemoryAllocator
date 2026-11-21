
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


#include "slab.h"
#include "heap.h"

#define SMAGIC		0x51AB51AB
#define SHDRSZ		32		/* sizeof(struct slab) padded */
#define NOBJ		0xFFFFFFFFu

struct slab {
	struct kmem_cache *c;
	struct slab	*next;
	uint32_t	free_head;
	uint32_t	nfree;
};

/* descriptors come from a static pool, not kmalloc -- creating a cache
 * must not recurse into the allocator it configures */
static struct kmem_cache	cpool[KCACHE_MAX];
static uint8_t			cused[KCACHE_MAX];

static spin_t	sl = SPIN_INIT;

struct kmem_cache *kmem_cache_create(const char *name, size_t osz, size_t al)
{
	struct kmem_cache *c = NULL;
	int i;

	if (!name || !osz || al > 16)
		return NULL;

	osz = (osz + 15) & ~15UL;
	if (osz < 16)
		osz = 16;

	spin_lock(&sl);
	for (i = 0; i < KCACHE_MAX; i++) {
		if (!cused[i]) {
			cused[i] = 1;
			c = &cpool[i];
			break;
		}
	}
	spin_unlock(&sl);
	if (!c)
		return NULL;

	if (osz > (PAGE_SIZE - SHDRSZ) / 2) {
		/* too big for a slab, caller wants kmalloc anyway */
		cused[c - cpool] = 0;
		return NULL;
	}

	memset(c, 0, sizeof(*c));
	for (i = 0; name[i] && i < KCNAME - 1; i++)
		c->name[i] = name[i];
	c->osz = osz;
	c->nper = (PAGE_SIZE - SHDRSZ) / osz;
	return c;
}

static struct slab *slab_new(struct kmem_cache *c)
{
	struct slab *s;
	uint8_t *objs;
	uint32_t i;

	s = page_va_alloc();		/* aligned + zeroed */
	if (!s)
		return NULL;

	s->c = c;
	s->nfree = c->nper;
	s->free_head = 0;

	/* free objects chain through their first 4 bytes */
	objs = (uint8_t *)s + SHDRSZ;
	for (i = 0; i < c->nper - 1; i++)
		*(uint32_t *)(objs + i * c->osz) = i + 1;
	*(uint32_t *)(objs + (c->nper - 1) * c->osz) = NOBJ;
	return s;
}

void *kmem_cache_alloc(struct kmem_cache *c)
{
	struct slab **l, *s;
	uint8_t *objs;
	uint32_t i;

	if (!c)
		return NULL;

	spin_lock(&sl);
	l = &c->partial;
	if (!*l)
		l = &c->empty;
	if (!*l) {
		s = slab_new(c);
		if (!s) {
			spin_unlock(&sl);
			return NULL;
		}
		s->next = c->empty;
		c->empty = s;
		l = &c->empty;
	}

	s = *l;
	objs = (uint8_t *)s + SHDRSZ;
	i = s->free_head;
	s->free_head = *(uint32_t *)(objs + i * c->osz);
	s->nfree--;

	if (!s->nfree) {
		*l = s->next;
		s->next = c->full;
		c->full = s;
	} else if (l == &c->empty) {
		*l = s->next;
		s->next = c->partial;
		c->partial = s;
	}
	spin_unlock(&sl);
	return objs + i * c->osz;
}

void kmem_cache_free(struct kmem_cache *c, void *obj)
{
	struct slab *s, **l;
	uint8_t *objs;
	uint32_t i;

	if (!c || !obj)
		return;

	/* slab header lives at page start; trust c but sanity-check the magic */
	s = (struct slab *)((uint64_t)obj & ~(PAGE_SIZE - 1));
	if (s->c != c || s->nfree >= c->nper)
		return;

	spin_lock(&sl);
	objs = (uint8_t *)s + SHDRSZ;
	i = (obj - (void *)objs) / c->osz;
	*(uint32_t *)(objs + i * c->osz) = s->free_head;
	s->free_head = i;
	s->nfree++;

	if (s->nfree == c->nper) {
		for (l = &c->partial; *l; l = &(*l)->next) {
			if (*l == s) {
				*l = s->next;
				s->next = c->empty;
				c->empty = s;
				break;
			}
		}
	}
	spin_unlock(&sl);
}

void kmem_cache_destroy(struct kmem_cache *c)
{
	struct slab *s, *lists[3];
	int i;

	if (!c)
		return;

	spin_lock(&sl);
	lists[0] = c->partial;
	lists[1] = c->full;
	lists[2] = c->empty;
	for (i = 0; i < 3; i++)
		for (s = lists[i]; s; s = s->next)
			page_va_free(s);
	c->partial = c->full = c->empty = NULL;
	cused[c - cpool] = 0;
	spin_unlock(&sl);
}
