

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



#include "vmm.h"
#include "page.h"

#ifdef __x86_64__

#define IDX(va, lvl)	(((va) >> (12 + 9 * (lvl))) & 0x1FF) /* 0=PT 1=PD 2=PDPT 3=PML4 */
#define PS_PAGE		(1ULL << 7)	/* PS bit: 1G at PML4/PDPT, 2M at PD */

typedef volatile uint64_t pte_t;

static uint64_t kpml4;

static inline void invlpg(uint64_t va)
{
	__asm__ volatile("invlpg (%0)" :: "r"(va) : "memory");
}

static inline uint64_t rcr3(void)
{
	uint64_t v;
	__asm__ volatile("mov %%cr3, %0" : "=r"(v));
	return v;
}

void vmm_init(void)
{
	kpml4 = rcr3() & ~0xFFFULL;
}

uint64_t vmm_cr3(void)
{
	return kpml4;
}

/* descend one level, allocating the table if needed */
static pte_t *descend(pte_t *t, size_t i, bool *newtab)
{
	uint64_t e = t[i];
	uint64_t pa;

	if (!(e & PTE_P)) {
		pa = pmm_alloc();
		if (!pa)
			return NULL;
		memset(p2v(pa), 0, PAGE_SIZE);
		t[i] = pa | PTE_P | PTE_W;
		if (newtab)
			*newtab = true;
	}
	return (pte_t *)p2v(e & ~0xFFFULL);
}

int vmm_map(uint64_t va, uint64_t pa, uint64_t fl)
{
	pte_t *t;
	bool dummy;

	if ((va | pa) & (PAGE_SIZE - 1))
		return -1;

	t = (pte_t *)p2v(kpml4);
	if (!(t = descend(t, IDX(va, 3), &dummy)))
		return -1;
	if (!(t = descend(t, IDX(va, 2), &dummy)))
		return -1;
	if (!(t = descend(t, IDX(va, 1), &dummy)))
		return -1;

	if (t[IDX(va, 0)] & PTE_P)
		return -1;		/* already mapped */
	t[IDX(va, 0)] = pa | fl;
	invlpg(va);
	return 0;
}

void vmm_unmap(uint64_t va)
{
	pte_t *t = (pte_t *)p2v(kpml4);
	uint64_t e;

	e = t[IDX(va, 3)];
	if (!(e & PTE_P))
		return;
	t = (pte_t *)p2v(e & ~0xFFFULL);
	e = t[IDX(va, 2)];
	if (!(e & PTE_P))
		return;
	t = (pte_t *)p2v(e & ~0xFFFULL);
	e = t[IDX(va, 1)];
	if (!(e & PTE_P))
		return;
	t = (pte_t *)p2v(e & ~0xFFFULL);

	t[IDX(va, 0)] = 0;
	invlpg(va);
	/* intermediate tables leak. fine for now, needs refcounting */
}

uint64_t vmm_walk(uint64_t va)
{
	pte_t *t = (pte_t *)p2v(kpml4);
	uint64_t e;

	e = t[IDX(va, 3)];
	if (!(e & PTE_P))
		return 0;
	if (e & PS_PAGE)
		return (e & ~0xFFFULL) + (va & 0x1FFFFFFFFFFF);
	t = (pte_t *)p2v(e & ~0xFFFULL);
	e = t[IDX(va, 2)];
	if (!(e & PTE_P))
		return 0;
	if (e & PS_PAGE)
		return (e & ~0xFFFULL) + (va & 0x3FFFFFFF);
	t = (pte_t *)p2v(e & ~0xFFFULL);
	e = t[IDX(va, 1)];
	if (!(e & PTE_P))
		return 0;
	if (e & PS_PAGE)
		return (e & ~0xFFFULL) + (va & 0x1FFFFF);
	t = (pte_t *)p2v(e & ~0xFFFULL);
	e = t[IDX(va, 0)];
	if (!(e & PTE_P))
		return 0;
	return (e & ~0xFFFULL) + (va & 0xFFF);
}

int vmm_map_range(uint64_t va, uint64_t pa, uint64_t len, uint64_t fl)
{
	uint64_t off = 0;

	while (off < len) {
		if (vmm_map(va + off, pa + off, fl)) {
			while (off) {
				off -= PAGE_SIZE;
				vmm_unmap(va + off);
			}
			return -1;
		}
		off += PAGE_SIZE;
	}
	return 0;
}

/* new as = fresh pml4, higher half copied from kernel */
uint64_t vmm_new_as(void)
{
	pte_t *n, *k;
	uint64_t pa;
	size_t i;

	pa = pmm_alloc();
	if (!pa)
		return 0;
	n = (pte_t *)p2v(pa);
	k = (pte_t *)p2v(kpml4);
	memset((void *)n, 0, PAGE_SIZE);
	for (i = 256; i < 512; i++)
		n[i] = k[i];
	return pa;
}

#else
#error vmm.c: no port for this arch
#endif
