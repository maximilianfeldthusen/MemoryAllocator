



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


#include "page.h"
#include "bitmap.h"

static bitmap_t	fbm;
static uint64_t	fbase;		/* pa of frame 0 */
static uint64_t	ramtop;

/* bump pool. the heap doesn't exist yet when this runs. */
static uint8_t	*eap;
static size_t	eal;

static void *ealloc(size_t sz, size_t al)
{
	uintptr_t b = ((uintptr_t)eap + al - 1) & ~(uintptr_t)(al - 1);
	void *p;

	if (b - (uintptr_t)eap > eal)
		return NULL;
	eal -= b - (uintptr_t)eap;
	eap = (uint8_t *)b;
	if (sz > eal)
		return NULL;
	p = eap;
	eap += sz;
	eal -= sz;
	return p;
}

void pmm_init(uint64_t rs, uint64_t re, uint64_t pool, uint64_t plen)
{
	uint64_t n = (re - rs) / PAGE_SIZE;

	fbase = rs;
	ramtop = re;
	eap = (uint8_t *)pool;
	eal = plen;

	bitmap_init(&fbm, ealloc((n + 7) / 8, 8), n);
}

void pmm_reserve(uint64_t pa, uint64_t len)
{
	uint64_t f, lo, hi;

	if (pa + len <= fbase)
		return;
	lo = pa > fbase ? (pa - fbase) / PAGE_SIZE : 0;
	hi = (pa + len - 1 - fbase) / PAGE_SIZE;
	for (f = lo; f <= hi && f < fbm.n; f++)
		if (!BIT(&fbm, f))
			SET(&fbm, f);
}

uint64_t pmm_alloc(void)
{
	size_t i = bitmap_scan(&fbm, 1);

	return i == SIZE_MAX ? 0 : fbase + (uint64_t)i * PAGE_SIZE;
}

uint64_t pmm_alloc_n(size_t n)
{
	size_t i = bitmap_scan(&fbm, n);

	return i == SIZE_MAX ? 0 : fbase + (uint64_t)i * PAGE_SIZE;
}

void pmm_free(uint64_t pa)
{
	size_t i;

	if (pa < fbase || pa >= ramtop || pa & (PAGE_SIZE - 1))
		return;
	i = (pa - fbase) / PAGE_SIZE;
	if (!BIT(&fbm, i))		/* double free, drop it */
		return;
	CLR(&fbm, i);
}

void pmm_free_n(uint64_t pa, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++)
		pmm_free(pa + i * PAGE_SIZE);
}


`page.c` uses the `BIT`/`SET`/`CLR` macros, so export them from `bitmap.h` instead of keeping them private in `bitmap.c`:


#define BIT(b, i)	((b)->b[(i) >> 3] & (1u << ((i) & 7)))
#define SET(b, i)	((b)->b[(i) >> 3] |= (1u << ((i) & 7)))
#define CLR(b, i)	((b)->b[(i) >> 3] &= ~(1u << ((i) & 7)))


