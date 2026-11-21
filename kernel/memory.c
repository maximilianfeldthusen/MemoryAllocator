



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



#include "../include/memory.h"
#include "../mm/page.h"
#include "../mm/vmm.h"
#include "../mm/heap.h"
#include "../mm/slab.h"

/* boot.S / platform code gives us mem_base, mem_top from the loader's
 * memory map. kernel_end comes from linker.ld. */

extern uint8_t kernel_end[];

void memory_init(uint64_t mem_base, uint64_t mem_top)
{
	uint64_t ke = (uint64_t)kernel_end;

	/* bitmap lives right behind the kernel image */
	pmm_init(mem_base, mem_top, ke + PAGE_SIZE,
		 ((mem_top - mem_base) / PAGE_SIZE + 7) / 8 + PAGE_SIZE);

	pmm_reserve(mem_base, ke - mem_base);

	vmm_init();
	heap_init();

	/* per-subsystem caches get created here, e.g.
	 * task_cache = kmem_cache_create("task", sizeof(task_t), 0); */
}
