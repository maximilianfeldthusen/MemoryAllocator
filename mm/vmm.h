

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



#ifndef MM_VMM_H
#define MM_VMM_H

#include "../include/memory.h"

void	vmm_init(void);
int	vmm_map(uint64_t va, uint64_t pa, uint64_t flags);
void	vmm_unmap(uint64_t va);
uint64_t vmm_walk(uint64_t va);		/* 0 if unmapped */
int	vmm_map_range(uint64_t va, uint64_t pa, uint64_t len, uint64_t flags);
uint64_t vmm_new_as(void);
uint64_t vmm_cr3(void);

#endif
