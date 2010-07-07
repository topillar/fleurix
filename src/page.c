
#include <param.h>
#include <x86.h>
#include <kern.h>

/**
 *  0x1000000   |---------------------------------------------- 16mb
 *              | 
 *              | Remapped physical memory, 
 *              | 
 *              | now HI_MEM, for user proc 
 *  0x100000    |---------------------------------------------- 1mb
 *              |
 *              | VGA, BIOS ROM and blah~
 *  0xa0000     |---------------------------------------------- 640kb
 *              | 
 *              |
 *              | kenel loads here
 *  0x10000     |---------------------------------------------- 64kb 
 *              |
 *              | boot.bin here. not used yet
 *  0x7c00      |---------------------------------------------- 31kb
 *              | 
 *              | page_dir and 4 page_tables, 20kb needed
 *  0x0000      |---------------------------------------------- 0
 *
 *
 *
 *
 * */

uint la2pa(uint la);

// there might be a TODO here... 16mb is so poor boy~
// the ONLY page directory, 
// which indicates the linear address space of 4GB
// each proc have a address space of 64MB, which starts at pid*64mb
uint *pdir = (uint *) 0x00000;

uchar frmmap[NFRAME] = {0, };

/**************************************************************/

void do_page_fault(struct regs *r){
}

void do_no_page(struct regs *r){
}

void do_wp_page(struct regs *r){
}

/**************************************************************/

int put_page(uint la, uint pa, uint flag){
    //printf("# %x -> %x\n", la, pa);
    uint *ptab = pdir[PDX(la)];
    if (!((uint)ptab & PTE_P)){
        ptab = palloc();
        if (ptab==0){
            panic("no availible frame");
        }
        ptab = ((uint)ptab) | flag;
        pdir[PDX(la)] = ptab;
    }
    ptab[PTX(la)] = pa | flag;
    frmmap[pa/4096]++;
    return 0;
}

int copy_page(uint la1, uint la2, uint limit){
}

// copy page tables, as a helper of copy_proc()
// it do NOT copy the data inside a frame, just remap it
// and mark it READONLY. 
// parameter src, dst, and limit are deserved multiple of 0x1000
int copy_ptab(uint src, uint dst, uint limit){
    int off, la, pa;
    for(off=0; off<=limit; off+=0x1000){
        pa = la2pa(src+off);
        put_page(dst+off, pa, PTE_P | PTE_W | PTE_U);
    }
    flush_cr3(pdir);
    return 0;
}

/**************************************************************/

void print_pdir(){
    uint la, pa;
    for(la=0x4000000; la<0x8000000; la+=0x1000){
        pa = la2pa(la);
        printf("%x -> %x\n", la, pa);   
    }
}

/**************************************************************/

// traverse a linear address to physical address
// TODO, have a check of PTE_P
uint la2pa(uint la){
    uint ret;
    ret = pdir[PDX(la)];
    if (!(ret & PTE_P)){
        panic("invalid pde\n");
    }
    uint *ptab = PTE_ADDR(ret);
    uint page  = PTE_ADDR(ptab[PTX(la)]);
    return page + POFF(la);
}

/**************************************************************/
// Allocate/Free a Frame

// tranverse frmmap, if 0, set it 1 and return the address
// if no frame availible, return 0
uint palloc(){
    int i;
    for(i=0; i<NFRAME; i++){
        if(frmmap[i]==0){
            frmmap[i] = 1;
            return LO_MEM + i*4096;
        }
    }
    return 0;
}

// decrease the target in frmmap. 
int pfree(uint addr){
    int i;
    i = (addr-LO_MEM) / 4096;
    if(frmmap[i]==0){
        return 0;
    }
    frmmap[i]--;
    return i;
}

/***************************************************************/

void flush_cr3(uint addr){
    asm volatile("mov %%eax, %%cr3":: "a"(addr));
}

void page_enable(){
    uint cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void page_init(){
    // address of a page directory have to make aligned to multiple of 4kb
    // the page table comes right after the page directory, 0x1000 == 1024 * 4
	uint *ptab;

	// map the top 16MB of memory
    //
    // FOUR page table, 16kb
    // fill the top FOUR entry of the page directory
    // attribute set to: supervisor level, read/write, present(011 in binary)
	uint i, j;
	uint addr = 0; 
    for(i=0; i<4; i++){
        ptab = 0x1000 + 0x1000*i;
        pdir[i] = (uint)ptab | 3;
        for(j=0; j<1024; j++) {
            ptab[j] = addr | PTE_P | PTE_W; 
            addr += 4096; // 4096 = 4kb
        }
    }

	// fill the rest of the page directory
    // attribute set to: supervisor level, read/write, not present(010 in binary)
	for(i=4; i<1024; i++) {
		pdir[i] = 0 | PTE_W; 
	};

    // write page directory to cr3 and enable PE on cr0
    flush_cr3(pdir);
    page_enable();
}

