

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



#ifndef MM_SLAB_H
#define MM_SLAB_H

#include "../include/memory.h"

#define KCACHE_MAX	32
#define KCNAME		24

struct slab;
struct kmem_cache;

/* typed caches for hot fixed-size objects. variable sizes go to kmalloc. */
struct kmem_cache {
	char		name[KCNAME];
	size_t		osz;		/* rounded to 16 */
	size_t		nper;		/* objects per slab */
	struct slab	*partial;
	struct slab	*full;
	struct slab	*empty;
};

struct kmem_cache *kmem_cache_create(const char *, size_t, size_t);
void	*kmem_cache_alloc(struct kmem_cache *);
void	kmem_cache_free(struct kmem_cache *, void *);
void	kmem_cache_destroy(struct kmem_cache *);

#endif


