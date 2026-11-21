


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



#include "bitmap.h"

#define BIT(b, i)	((b)->b[(i) >> 3] & (1u << ((i) & 7)))
#define SET(b, i)	((b)->b[(i) >> 3] |= (1u << ((i) & 7)))
#define CLR(b, i)	((b)->b[(i) >> 3] &= ~(1u << ((i) & 7)))

void bitmap_init(bitmap_t *bm, void *s, size_t n)
{
	bm->b = s;
	bm->n = n;
	memset(s, 0, (n + 7) / 8);
}

void bitmap_set(bitmap_t *bm, size_t i) { SET(bm, i); }
void bitmap_clr(bitmap_t *bm, size_t i) { CLR(bm, i); }
int bitmap_tst(const bitmap_t *bm, size_t i) { return !!BIT(bm, i); }

/* linear scan. word-wise later if this ever shows up in profiles. */
size_t bitmap_scan(bitmap_t *bm, size_t n)
{
	size_t run = 0, start = 0, i;

	if (!n || n > bm->n)
		return SIZE_MAX;

	for (i = 0; i < bm->n; i++) {
		if (!BIT(bm, i)) {
			if (!run)
				start = i;
			if (++run == n) {
				for (i = start; i < start + n; i++)
					SET(bm, i);
				return start;
			}
		} else
			run = 0;
	}
	return SIZE_MAX;
}

size_t bitmap_nset(const bitmap_t *bm)
{
	size_t i, c = 0;

	for (i = 0; i < bm->n; i++)
		if (BIT(bm, i))
			c++;
	return c;
}
