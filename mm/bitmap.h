



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



#ifndef MM_BITMAP_H
#define MM_BITMAP_H

#include "../include/memory.h"

typedef struct {
	uint8_t *b;
	size_t	n;
} bitmap_t;

void	bitmap_init(bitmap_t *, void *, size_t);
size_t	bitmap_scan(bitmap_t *, size_t);	/* n clear bits, set, ret first or SIZE_MAX */
void	bitmap_set(bitmap_t *, size_t);
void	bitmap_clr(bitmap_t *, size_t);
int	bitmap_tst(const bitmap_t *, size_t);
size_t	bitmap_nset(const bitmap_t *);

#endif
