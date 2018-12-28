#include "pc.h"
#include <driver/vga.h>
#include <intr.h>
#include <arch.h>
#include <zjunix/syscall.h>
#include <zjunix/log.h>

unsigned int sysctl_sched_latency = 1000000;
task_struct *current_task = 0;
struct list_head all_task;
struct list_head all_dead;
struct list_head all_waiting;
struct list_head all_ready;
unsigned int cur_PID = 0;

void add_task(task_struct *task, struct list_head tasks) {
    list_add_tail(&(task->task), &tasks);
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
    init_CFS();
    // sysctl_sched_latency = 1000000;
    INIT_LIST_HEAD(&all_task);
    INIT_LIST_HEAD(&all_dead);
    INIT_LIST_HEAD(&all_waiting);
    INIT_LIST_HEAD(&all_ready);
    task_union *new = (task_union*)(kernel_sp - TASK_KERNEL_SIZE);
    new->task.PID = cur_PID++;
    new->task.parent = 0;
    new->task.state = 0;
    INIT_LIST_HEAD(&(new->task.task));
    kernel_strcpy(new->task.name, "idle");
    register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, pc_schedule);
    // asm volatile(   // huge bug here, piece of shit...
    //     //"li $v0, %0\n\t"
    //     "mtc0 $0, $11\n\t"
    //     "mtc0 $zero, $9"
    //     : : "r"(sysctl_sched_latency));
    asm volatile(
        "li $v0, 1000000\n\t"
        "mtc0 $v0, $11\n\t"
        "mtc0 $zero, $9");
    current_task = &(new->task);
    add_task(&(new->task), all_task);
}

// change the reschedule period of CFS by modifying the interrupt period
void change_sysctl_sched_latency(unsigned int latency){
    asm volatile(
        "mtc0 $0, $11\n\t"
        "mtc0 $zero, $9"
        : : "r"(latency));
}

void pc_schedule(unsigned int status, unsigned int cause, context* pt_context) {

    fair_sched_class.update_vruntime();

    copy_context(pt_context, &(current_task->context));
    task_struct *next = current_task;
    copy_context(&(next->context), pt_context);
    current_task = next;
    asm volatile("mtc0 $zero, $9\n\t");

}


void pc_create(char *task_name, void(*entry)(unsigned int argc, void *args), unsigned int argc, void *args) {
    task_union *new = (task_union*) kmalloc(sizeof(task_union));
    INIT_LIST_HEAD(&(new->task.task));
    kernel_strcpy(new->task.name, task_name);
    // ------- setting context registers
    kernel_memset(&(new->task.context), 0, sizeof(context));
    new->task.context.epc = (unsigned int)entry;
    new->task.context.sp = (unsigned int)new + TASK_KERNEL_SIZE;
    unsigned int init_gp;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp)); 
    new->task.context.gp = init_gp;
    new->task.context.a0 = argc;
    new->task.context.a1 = (unsigned int)args;
    // ------- done setting context registers
    new->task.PID = cur_PID++;
    new->task.parent = current_task->PID;
    new->task.state = 0;
    add_task(&(new->task), all_task);
}

void pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context) {

    // pc_kill(current_task);
    // pc_schedule(status, cause, pt_context);
    
}

// kill a process by PID
int pc_kill(unsigned int PID) {

    return 0;
}


int print_proc() {
    struct list_head *pos;
    task_struct *next;
    list_for_each(pos, &all_task) {
        next = container_of(pos, task_struct, task);
        kernel_printf("PID : %d, name : %s, vruntime : %d\n", next->PID, next->name,
        next->sched_entity.vruntime);
    }
    return 0;
}
