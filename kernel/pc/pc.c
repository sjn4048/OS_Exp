#include "pc.h"
#include <driver/vga.h>
#include <intr.h>
#include <arch.h>
#include <zjunix/syscall.h>
#include <zjunix/log.h>

#define TASK_RUNNING 0
#define TASK_WAITING 1
#define TASK_READY 2
#define TASK_DEAD 3
task_struct *other = 0;
unsigned int sysctl_sched_latency = 1000000;
task_struct *current_task = 0;
struct list_head all_task;
struct list_head all_dead;
struct list_head all_waiting;
struct list_head all_ready;
unsigned int cur_PID = 0;
struct cfs_rq rq;
void add_task(task_struct *task, struct list_head tasks) {
    list_add_tail(&(task->task_list), &tasks);
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
    // sysctl_sched_latency = 1000000;
    init_cfs_rq(&rq);
    INIT_LIST_HEAD(&all_task);
    INIT_LIST_HEAD(&all_dead);
    INIT_LIST_HEAD(&all_waiting);
    INIT_LIST_HEAD(&all_ready);
    task_union *new = (task_union*)(kernel_sp - TASK_KERNEL_SIZE);
    new->task.PID = cur_PID++;
    new->task.parent = 0;
    new->task.state = 0;
    INIT_LIST_HEAD(&(new->task.task_list));
    kernel_strcpy(new->task.name, "idle");
    register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, pc_schedule);
    // asm volatile(   // huge bug here, piece of shit...
    //     //"li $v0, %0\n\t"
    //     "mtc0 $0, $11\n\t"
    //     "mtc0 $zero, $9"
    //     : : "r"(sysctl_sched_latency));
    current_task = &(new->task);
    new->task.sched_entity.vruntime = 0;
    add_task(&(new->task), all_task);
    // add_task(&(new->task), all_ready);
    asm volatile(
        "li $v0, 1000000\n\t"
        "mtc0 $v0, $11\n\t"
        "mtc0 $zero, $9");
}

// change the reschedule period of CFS by modifying the interrupt period
void change_sysctl_sched_latency(unsigned int latency){
    asm volatile(
        "mtc0 $0, $11\n\t"
        "mtc0 $zero, $9"
        : : "r"(latency));
}

void pc_schedule(unsigned int status, unsigned int cause, context* pt_context) {
    
    update_vruntime_fair(current_task);
    copy_context(pt_context, &(current_task->context));
    task_struct *next = other;
    other = current_task;
    copy_context(&(next->context), pt_context);
    current_task = next;
    asm volatile("mtc0 $zero, $9\n\t");

}


void pc_create(char *task_name, void(*entry)(unsigned int argc, void *args), unsigned int argc, void *args, int nice) {
    task_union *new = (task_union*) kmalloc(sizeof(task_union));
    task_struct * task = &(new->task);
    kernel_strcpy(task->name, task_name);
    INIT_LIST_HEAD(&(task->task_list));
    INIT_LIST_HEAD(&(task->children));
    task->nice = nice;
    task->static_prio = nice + 20;
    task->normal_prio = nice + 20;
    task->PID = cur_PID++;
    task->usage = 0;

    // ------- setting schedule entity
    struct sched_entity* entity = &(task->sched_entity);
    entity->vruntime = 0;
    entity->exec_start_time = -1;
    entity->sum_exec_runtime = 0;
    entity->load.weight = prio_to_weight[task->normal_prio];
    entity->load.inv_weight = prio_to_wmult[task->normal_prio];
    // ------- done setting schedule entity

    // ------- setting context registers
    kernel_memset(&(task->context), 0, sizeof(context));
    task->context.epc = (unsigned int)entry;
    task->context.sp = (unsigned int)new + TASK_KERNEL_SIZE;
    unsigned int init_gp;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp)); 
    task->context.gp = init_gp;
    task->context.a0 = argc;
    task->context.a1 = (unsigned int)args;
    // ------- done setting context registers

    // task's parent is current task (who create it) 
    task->parent = current_task->PID;

    // add new task to parents' children list
    // list_add_tail(&(task->task), &(current_task->children));

    task->state = TASK_READY;
    // add to coresponding task queue(s)    
    kernel_printf("%d\n", (unsigned int)&(task->task_list));
    add_task(task, all_task);
    other = task;
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
    for (pos = (&all_task)->next; pos != (&all_task); pos = pos->next)
     {
        kernel_printf("%d\n", (unsigned int)&(pos));
        // next = container_of(pos, task_struct, task_list);
        // kernel_printf("  PID : %d, name : %s, vruntime : %d\n", next->PID, next->name,
        // next->sched_entity.vruntime);
    }
    return 0;
}
