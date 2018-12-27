#include "pc.h"
#include <driver/vga.h>
#include <intr.h>
#include <arch.h>
#include <zjunix/syscall.h>
#include <zjunix/log.h>

unsigned int sysctl_sched_latency = 1000000;
task_struct *current_task = 0;
struct list_head tasks;
unsigned int cur_PID = 0;
void add_task(task_struct *task) {
    list_add_tail(&(task->sched), &tasks);
}
static void copy_context(context* src, context* dest) {
    dest->epc = src->epc;
    dest->at = src->at;
    dest->v0 = src->v0;
    dest->v1 = src->v1;
    dest->a0 = src->a0;
    dest->a1 = src->a1;
    dest->a2 = src->a2;
    dest->a3 = src->a3;
    dest->t0 = src->t0;
    dest->t1 = src->t1;
    dest->t2 = src->t2;
    dest->t3 = src->t3;
    dest->t4 = src->t4;
    dest->t5 = src->t5;
    dest->t6 = src->t6;
    dest->t7 = src->t7;
    dest->s0 = src->s0;
    dest->s1 = src->s1;
    dest->s2 = src->s2;
    dest->s3 = src->s3;
    dest->s4 = src->s4;
    dest->s5 = src->s5;
    dest->s6 = src->s6;
    dest->s7 = src->s7;
    dest->t8 = src->t8;
    dest->t9 = src->t9;
    dest->hi = src->hi;
    dest->lo = src->lo;
    dest->gp = src->gp;
    dest->sp = src->sp;
    dest->fp = src->fp;
    dest->ra = src->ra;
}

void init_pc() {
    INIT_LIST_HEAD(&tasks);
    task_union *new = (task_union*)(kernel_sp - TASK_KERNEL_SIZE);
    new->task.PID = cur_PID++;
    new->task.parent = 0;
    new->task.state = 0;
    INIT_LIST_HEAD(&(new->task.sched));
    kernel_strcpy(new->task.name, "idle");
    log(LOG_STEP, "1");
    register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, pc_schedule);
    log(LOG_STEP, "2");
    asm volatile(
        //"li $v0, %0\n\t"
        "mtc0 $0, $11\n\t"
        "mtc0 $zero, $9"
        : : "r"(sysctl_sched_latency));
}

//改变cfs调度的周期，通过修改中断时间间隔实现
void change_sysctl_sched_latency(unsigned int latency){
    asm volatile(
        //"li $v0, %0\n\t"
        "mtc0 $0, $11\n\t"
        "mtc0 $zero, $9"
        : : "r"(latency));
}

void pc_schedule(unsigned int status, unsigned int cause, context* pt_context) {
    struct list_head *pos;
    task_struct *next, *torun, *towait;
    copy_context(pt_context, &(current_task->context));
    list_for_each(pos, &tasks) {
        next = container_of(pos, task_struct, sched);
        if (next->name != current_task->name) {
            // Load context
            copy_context(&(next->context), pt_context);
            current_task = next;
        }
    }

    asm volatile("mtc0 $zero, $9\n\t");
}

int pc_peek() {
    // int i = 0;
    // for (i = 0; i < 8; i++)
    //     if (pcb[i].ASID < 0)
    //         break;
    // if (i == 8)
    //     return -1;
    // return i;
    return 0;
}

void pc_create(char *task_name, void(*entry)(unsigned int argc, void *args), unsigned int argc, void *args) {
    task_union *new = (task_union*) kmalloc(sizeof(task_union));
    log(LOG_STEP, "1");
    kernel_memset(&(new->task.context), 0, sizeof(context));
    log(LOG_STEP, "2");
    kernel_strcpy(new->task.name, task_name);
    log(LOG_STEP, "3");
    new->task.context.epc = (unsigned int)entry;
    log(LOG_STEP, "4");
    INIT_LIST_HEAD(&(new->task.sched));
    log(LOG_STEP, "5");
    new->task.context.sp = (unsigned int)new + TASK_KERNEL_SIZE;
    log(LOG_STEP, "6");
    unsigned int init_gp;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp)); 
    new->task.context.gp = init_gp;
    log(LOG_STEP, "7");
    new->task.context.a0 = argc;
    new->task.context.a1 = (unsigned int)args;
    new->task.PID = cur_PID++;
    new->task.parent = current_task->PID;
    new->task.state = 0;
    log(LOG_STEP, "8");
    add_task(&(new->task));
}

void pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context) {
    if (current_task->state == 0) {
        current_task->state = 1;
        pc_schedule(status, cause, pt_context);
    }
}

int pc_kill(int proc) {
    return 0;
    // proc &= 7;
    // if (proc != 0 && pcb[proc].ASID >= 0) {
    //     pcb[proc].ASID = -1;
    //     return 0;
    // } else if (proc == 0)
    //     return 1;
    // else
    //     return 2;
}

task_struct* get_curr_pcb() {
    // return &pcb[curr_proc];
    return current_task;
}

int print_proc() {
    // int i;
    // kernel_puts("PID name\n", 0xfff, 0);
    // for (i = 0; i < 8; i++) {
    //     if (pcb[i].ASID >= 0)
    //         kernel_printf(" %x  %s\n", pcb[i].ASID, pcb[i].name);
    // }
    return 0;
}
