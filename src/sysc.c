
#include <param.h>
#include <x86.h>
#include <kern.h>
#include <sysc.h>

int errno = 0;

/***********************************************************/

void sys_null(){
    ;
}

void sys_fork(struct regs *r){
    int ret;
    ret = copy_proc(r);
    if (ret<0){
        panic("error fork()\n");
    }
    r->eax = 1;
}

_syscall0(0, int, null);
_syscall0(1, int, fork);

/***********************************************************/

static uint sys_routines[NSYSC] = {
    [0] = &sys_null,
    [1] = &sys_fork,
    0,
}; 

void do_syscall(struct regs *r){
    void (*handler)(struct regs *r);
    handler = sys_routines[r->eax];
    
    if(handler){
        handler(r);
    }
}

