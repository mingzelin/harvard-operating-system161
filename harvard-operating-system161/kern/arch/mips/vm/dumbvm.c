/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground.
 */

/* under dumbvm, always have 48k of user stack */
#define DUMBVM_STACKPAGES    12

/*
 * Wrap rma_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;


void
vm_bootstrap(void) {
        paddr_t lo;
        paddr_t hi;
        paddr_t curr;
        ram_getsize(&lo, &hi);
        int blanksize = hi - lo;


        cmap = (struct coremap *) PADDR_TO_KVADDR(lo);

        frame_total_num = blanksize / PAGE_SIZE;
        curr = lo + frame_total_num * sizeof(struct coremap);
        //padding the area reserved for cmap
        curr = curr + (PAGE_SIZE - (curr % PAGE_SIZE));
        reserved_num = (curr - lo) / PAGE_SIZE;

        //initialize the cmap (array not include cmap itself)
        int index = 0;
        for (; curr < hi; curr += PAGE_SIZE) {
                KASSERT(index <= (frame_total_num - reserved_num));
                cmap[index].component = 0;
                cmap[index].occupied = 0;
                cmap[index].addr = curr;
                index++;
        }
        has_coremap = 1;
}

static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	int pagenum = (int) npages;
	int entrynum = frame_total_num - reserved_num;
	//
	bool special = false;
	int checkedpages = 0;
	paddr_t found = -1;
	//find the space to fit
	if(has_coremap) {
		for (int i = 0; i < entrynum; i += checkedpages) {
			//kprintf("%d\n", cmap[i].occupied);
			if (cmap[i].occupied) {
				checkedpages = 1;
			} else {
				//find an empty spot check whether the space is big enough
				checkedpages = 1;
				if (pagenum == 1) { //found enough empty blocks
					found = i;
					special = true;
				}
				for (int j = 1; j < pagenum; j++) {
					checkedpages++;
					if (cmap[i + j].occupied == 0) { //unoccupied
						if (j == pagenum - 1) { //found enough empty blocks
							found = i;
							special = true;
							break;
						}

					} else { //occupied
						break;
					}
				}
			}
			if(special){
				break;
			}
		}

		if (found!= (paddr_t) -1) {
			//cmap[found].occupied = 9;
			cmap[found].occupied = 1;
			//cmap[found].occupied = 9;
			cmap[found].component = -1;//this is head
			addr = cmap[found].addr;
			for (int i = 1; i < pagenum; i++) {
				cmap[i + found].occupied = 1;
				//cmap[found].occupied = 9;
				cmap[i + found].component = 1;
			}
		} else {
			spinlock_release(&stealmem_lock);
			panic("out of page (from getppages in dumbvm.c)");
			return ENOMEM;
		}
		spinlock_release(&stealmem_lock);
		return addr;
	}
	addr = ram_stealmem(npages);
	spinlock_release(&stealmem_lock);
	return addr;
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t 
alloc_kpages(int npages)
{
	paddr_t pa;
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)
{
	spinlock_acquire(&stealmem_lock);

	if(addr == 0){
		spinlock_release(&stealmem_lock);
		panic("invalid addr 0 from free_kpages() in dumbvm.c");
		return;
	}
	int pagenum = frame_total_num - reserved_num;


	int index = -1;

	for(int i=0; i<pagenum;i++){
		if(addr == PADDR_TO_KVADDR(cmap[i].addr)){
			index = i;
			//beginning index is found
			break;
		}
	}

	if(index == -1){
		//panic("cannot find the addr from free_kpages() in dumbvm.c");
		spinlock_release(&stealmem_lock);
		return;
	}


	cmap[index].occupied = 0;
	cmap[index].component = 0;
	index++;
	while((cmap[index].occupied == 1)&&(cmap[index].component == 1)){
		//continue when:
		// 1. the next is occupied and component == 1
		cmap[index].occupied = 0;
		cmap[index].component = 0;
		index++;
	}

	spinlock_release(&stealmem_lock);
	return;
}



void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t vbase1, vtop1, vbase2, vtop2, stackbase, stacktop;
	paddr_t paddr;
	int i;
        uint32_t ehi, elo;
        struct addrspace *as;
        int spl;
	int isCode = 0;

	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		return EFAULT;
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}

	as = curproc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	/* Assert that the address space has been set up properly. */
	KASSERT(as->as_vbase1 != 0);
	KASSERT(as->pt1 != NULL);
	KASSERT(as->as_npages1 != 0);
	KASSERT(as->as_vbase2 != 0);
	KASSERT(as->pt2 != NULL);
	KASSERT(as->as_npages2 != 0);
	//KASSERT(as->pt3 != NULL);
	KASSERT((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
//	KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	KASSERT((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
//	KASSERT((as->as_pbase2 & PAGE_FRAME) == as->as_pbase2);
//	KASSERT((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	vbase1 = as->as_vbase1;
	vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
	vbase2 = as->as_vbase2;
	vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;

	if (faultaddress >= vbase1 && faultaddress < vtop1) {//code
		isCode = 1;
		int num = (faultaddress - vbase1)/PAGE_SIZE;
		paddr = as->pt1[num].ptaddr;
	}
	else if (faultaddress >= vbase2 && faultaddress < vtop2) {//data
		int num = (faultaddress - vbase2)/PAGE_SIZE;
		paddr = as->pt2[num].ptaddr;
	}
	else if (faultaddress >= stackbase && faultaddress < stacktop) {//stack
		int num = (faultaddress - stackbase)/PAGE_SIZE;
		paddr = as->pt3[num].ptaddr;
	}
	else {
		return EFAULT;
	}

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;

		if(isCode && as->hasElfLoaded){
			elo = elo & ~TLBLO_DIRTY;
		}


		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}
	ehi = faultaddress;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	if(isCode && as->hasElfLoaded){
		elo = elo & ~TLBLO_DIRTY;
	}
	tlb_random(ehi, elo);

	splx(spl);
	return 0;
}




struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}
	as->as_vbase1 = 0;
	as->pt1 = NULL;
	as->as_npages1 = 0;
	as->as_vbase2 = 0;
	as->pt2 = NULL;
	as->as_npages2 = 0;
	as->pt3 = NULL;
	as->hasElfLoaded = 0;
	return as;
}

void
as_destroy(struct addrspace *as)
{
	struct page* curr = as->pt1;
	for (size_t i = 0; i< as->as_npages1; ++i) {
		free_kpages(PADDR_TO_KVADDR(as->pt1[i].ptaddr));
	}

	curr = as->pt2;
	for (size_t i = 0; i< as->as_npages2; ++i) {
		free_kpages(PADDR_TO_KVADDR(as->pt2[i].ptaddr));
	}

	curr = as->pt3;
	for (size_t i = 0; i< DUMBVM_STACKPAGES; ++i) {
		free_kpages(PADDR_TO_KVADDR(as->pt3[i].ptaddr));
	}

	kfree(as->pt1);
	kfree(as->pt2);
	kfree(as->pt3);
	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = curproc_getas();
#ifdef UW
        /* Kernel threads don't have an address spaces to activate */
#endif
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/* nothing */
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;//find offset
	vaddr &= PAGE_FRAME;//page number

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;

	/* We don't use these - all pages are read-write */
	(void)readable;
	(void)writeable;
	(void)executable;

	if (as->as_vbase1 == 0) {
		as->as_vbase1 = vaddr;
		as->as_npages1 = npages;
		return 0;
	}


	if (as->as_vbase2 == 0) {
		as->as_vbase2 = vaddr;
		as->as_npages2 = npages;
		return 0;
	}
	//stack is empty when init

	/*
	 * Support for more than two regions is not available.
	 */
	kprintf("dumbvm: Warning: too many regions\n");
	return EUNIMP;
}

static
void
as_zero_region(paddr_t paddr, unsigned npages)
{
	bzero((void *)PADDR_TO_KVADDR(paddr), npages * PAGE_SIZE);
}

//   as_prepare_load - this is called before actually loading from an
//               executable into the address space.
int as_prepare_load(struct addrspace *as)
{

	as->pt1 = kmalloc(as->as_npages1 * sizeof(struct page));
	as->pt2 = kmalloc(as->as_npages2 * sizeof(struct page));
	as->pt3 = kmalloc(DUMBVM_STACKPAGES * sizeof(struct page));
	struct page* curr = as->pt1;
	for (size_t i = 0; i < as->as_npages1; ++i) {
		curr[i].ptaddr = getppages(1);
		as_zero_region(curr[i].ptaddr, 1);
	}

	curr = as->pt2;
	for (size_t i = 0; i < as->as_npages2; ++i) {
		curr[i].ptaddr = getppages(1);
		as_zero_region(curr[i].ptaddr, 1);
	}


	curr = as->pt3;
	for (size_t i = 0; i < DUMBVM_STACKPAGES; ++i) {
		curr[i].ptaddr = getppages(1);
		as_zero_region(curr[i].ptaddr, 1);
	}
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT(as->pt3);
	*stackptr = USERSTACK;
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}
	//stack is empty when init
	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;


	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	KASSERT((new->pt1)&&(new->pt2)&&(new->pt3));

	struct page* curr = new->pt1;
	for (size_t i = 0; i< new->as_npages1; ++i) {
		memmove((void *)PADDR_TO_KVADDR(curr[i].ptaddr),
			(const void *)PADDR_TO_KVADDR(old->pt1[i].ptaddr),
			PAGE_SIZE);
	}

	curr = new->pt2;
	for (size_t i = 0; i< new->as_npages2; ++i) {
		memmove((void *)PADDR_TO_KVADDR(curr[i].ptaddr),
			(const void *)PADDR_TO_KVADDR(old->pt2[i].ptaddr),
			PAGE_SIZE);
	}

	curr = new->pt3;
	for (size_t i = 0; i< DUMBVM_STACKPAGES; ++i) {
		memmove((void *)PADDR_TO_KVADDR(curr[i].ptaddr),
			(const void *)PADDR_TO_KVADDR(old->pt3[i].ptaddr),
			PAGE_SIZE);
	}

	*ret = new;
	return 0;
}
