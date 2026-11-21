

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


#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))

/* va layout, x86_64 */
#define KVBASE		0xFFFFFFFF80000000ULL	/* kernel image */
#define PHYSMAP		0xFFFF888000000000ULL	/* direct map */
#define HPAGE_BASE	0xFFFFFF0000000000ULL	/* whole pages (slab) */
#define HBLK_BASE	0xFFFFFC0000000000ULL	/* free list */

#define PTE_P		(1ULL << 0)
#define PTE_W		(1ULL << 1)
#define PTE_U		(1ULL << 2)
#define PTE_NX		(1ULL << 63)
#define PTE_ADDR(x)	((x) & 0x000FFFFFFFFFF000ULL)

#define KDATA		(PTE_P | PTE_W | PTE_NX)
#define KCODE		(PTE_P | PTE_W)

typedef struct { volatile int v; } spin_t;
#define SPIN_INIT	{ 0 }

static inline void spin_lock(spin_t *l)
{
	while (__atomic_test_and_set(&l->v, __ATOMIC_ACQUIRE))
		;
}

static inline void spin_unlock(spin_t *l)
{
	__atomic_clear(&l->v, __ATOMIC_RELEASE);
}

static inline void *memset(void *d, int c, size_t n)
{
	uint8_t *p = d;
	while (n--)
		*p++ = (uint8_t)c;
	return d;
}

static inline void *memcpy(void *d, const void *s, size_t n)
{
	uint8_t *dp = d;
	const uint8_t *sp = s;
	while (n--)
		*dp++ = *sp++;
	return d;
}

static inline void *p2v(uint64_t pa)
{
	return (void *)(PHYSMAP + pa);
}

static inline uint64_t v2p(const void *va)
{
	return (uint64_t)va - PHYSMAP;
}

#endif

