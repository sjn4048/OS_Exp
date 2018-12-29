/*
 *  pc/pc.c
 *
 *  Kernel scheduler and related syscalls
 *
 *  Copyright (C) 2018  Zhenxin Xiao, Zhejiang University
 * 
 */

#include "pc.h"
#include <driver/vga.h>
#include <intr.h>
#include <arch.h>
#include <zjunix/syscall.h>
#include <zjunix/log.h>

// defines the task's possible state
#define TASK_RUNNING 0
#define TASK_WAITING 1
#define TASK_READY 2
#define TASK_DEAD 3

// intervl between two schedule interrupts
unsigned int sysctl_sched_latency = 1000000;

//current task pointer
task_struct *current_task = 0;

// lists contain different kinds of tasks
struct list_head all_task;
struct list_head all_dead;
struct list_head all_waiting;
struct list_head all_ready;

// PID allocating system
unsigned int cur_PID = 0;

// cfs running queue structure
struct cfs_rq rq;

// delete a task into list
#define remove_task(task, pointer) do {  \
    list_del(&((task)->pointer));        \
    INIT_LIST_HEAD(&((task)->pointer));  \
} while(0)

// copy register context
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

/* init_pc : 
 * initialize the process schedule module
 */
void init_pc() {
    // sysctl_sched_latency = 1000000;
    init_cfs_rq(&rq);

    INIT_LIST_HEAD(&all_task);
    INIT_LIST_HEAD(&all_dead);
    INIT_LIST_HEAD(&all_waiting);
    INIT_LIST_HEAD(&all_ready);


    // ---------------------------------------------------
    // ---------- setting idle task ----------------------
    task_union *new = (task_union*)(kernel_sp - TASK_KERNEL_SIZE);
    task_struct * task = &(new->task);

    // allocate a PID
    task->PID = cur_PID++;
    // idle task does not has parent
    task->parent = -1;
    task->state = TASK_READY;

    INIT_LIST_HEAD(&(task->task_list));
    INIT_LIST_HEAD(&(task->children));

    kernel_strcpy(task->name, "idle");

    // default nice value
    task->nice = 0; 
    task->static_prio = task->nice + 20;
    task->normal_prio = task->nice + 20;

    task->PID = cur_PID++;
    task->usage = 0;

    // ------- setting schedule entity
    sched_entity* entity = &(task->sched_entity);
    entity->vruntime = 0;
    entity->exec_start_time = -1;
    entity->sum_exec_runtime = 0;
    entity->load.weight = prio_to_weight[task->normal_prio];
    entity->load.inv_weight = prio_to_wmult[task->normal_prio];
    // ------- done setting schedule entity

    list_add_tail(&(new->task.task_list),&all_task);
    insert_process(&(rq.tasks_timeline),&(task->sched_entity));
    // ---------- done setting idle task -----------------
    // ---------------------------------------------------

    register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, pc_schedule);
    current_task = &(new->task);
    /* dynamicly defines the interrupt period by 
     * modifying the CP0's 11th register
     */
    asm volatile(
        "mtc0 %0, $11\n\t"
        "mtc0 $zero, $9"
        : : "r"(sysctl_sched_latency));
    // asm volatile(
    //     "li $v0, 1000000\n\t"
    //     "mtc0 $v0, $11\n\t"
    //     "mtc0 $zero, $9");
}

/* change_sysctl_sched_latency : 
 * change the reschedule period of CFS by modifying the interrupt period
 * it is done by modifying the CP0's 11th register
 */
void change_sysctl_sched_latency(unsigned int latency){
    asm volatile(
        "mtc0 %0, $11\n\t"
        "mtc0 $zero, $9"
        : : "r"(latency));
}

/* pc_schedule : 
 * process schedule entry, invoked by interrupts 
 * schedule interval is defined by sysctl_sched_latency
 * and can be modified by function 'change_sysctl_sched_latency'
 * it updates the vruntime of current task and choose the next
 * task which has the mininum vruntime
 */
void pc_schedule(unsigned int status, unsigned int cause, context* pt_context) {

    disable_interrupts();
    
    // update vruntime of current task
    update_vruntime_fair(&(rq),&(current_task->sched_entity),&(all_task),1);

    // find next task to run
    sched_entity *entity = pick_next_task_fair(&(rq));

    // if no change at all, just skip it and return
    if (entity == &(current_task->sched_entity)){
        asm volatile("mtc0 $zero, $9\n\t");
        enable_interrupts();
        return;
    }
    task_struct * next = container_of(entity, task_struct, sched_entity);

    // switch registers between current task and next one
    copy_context(pt_context, &(current_task->context));
    copy_context(&(next->context), pt_context);

    // change states
    current_task->state = TASK_READY;
    next->state = TASK_RUNNING;
    current_task = next;

    asm volatile("mtc0 $zero, $9\n\t");
    enable_interrupts();

}

/* pc_create : 
 * create a process 
 */
void pc_create(char *task_name, void(*entry)(unsigned int argc, void *args), unsigned int argc, void *args, int nice) {

    task_union *new = (task_union*) kmalloc(sizeof(task_union));
    task_struct * task = &(new->task);
    kernel_strcpy(task->name, task_name);
    INIT_LIST_HEAD(&(task->task_list));
    INIT_LIST_HEAD(&(task->children));

    // sets nice values and coresponding priorities
    task->nice = nice;
    task->static_prio = nice + 20;
    task->normal_prio = nice + 20;

    task->PID = cur_PID++;
    task->usage = 0;

    // ------- setting schedule entity
    sched_entity* entity = &(task->sched_entity);

    /* ---------------------- NOTE ---------------------- : 
     * the vruntime of new created task must be larger than its parent
     * due to the following two reasons:
     *     1. a process can not gain more CPU time by creating sub processes
     *     2. if vruntime starts with 0, it will block all other processes from 
     *        running because it takes some time for the new process to catch up
     *        its vruntime with the others
     */
    entity->vruntime = current_task->sched_entity.vruntime;

    entity->exec_start_time = -1;
    entity->sum_exec_runtime = 0;
    // get pre-calculated weight by nice value
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
    list_add_tail(&(task->task_list),&all_task);
    insert_process(&(rq.tasks_timeline),&(task->sched_entity));
}

/* pc_kill_syscall : 
 * process kill system call, it can be used to forcely terminate 
 * a process when a unhandled exception occurs
 * see more info in 'exc.c'
 */
void pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context) {

    pc_exit();
    pc_schedule(status, cause, pt_context);
    
}

/* pc_exit : 
 * kill itself (current running process)
 */
int pc_exit(){

}


/* pc_kill : 
 * kill a process by PID
 */
int pc_kill(unsigned int PID) {

    // can't kill idle process
    if (PID == 0) {
        kernel_printf("You are killing idle process!!!\n");
        return 1;
    }

    // can't kill itself
    if (PID == current_task->PID) {
        kernel_printf("You can't kill current task!\n");
        return 1;
    }

    disable_interrupts();
    // find that task by PID
    struct list_head *pos;
    task_struct *task;
    int find = 0;
    list_for_each(pos, (&all_task)) {
        task = container_of(pos, task_struct, task_list);
        if (task->PID == PID){
            find = 1;
            break;
        }
    }

    if (find == 0){
        kernel_printf("Process not found!\n");
        return 1;
    }

    task->state = TASK_DEAD;

    // clean up the task queue
    remove_task(task, task_list);
    delete_process(&(rq.tasks_timeline), &(task->sched_entity));

    enable_interrupts();
    return 0;

}

/* print_proc : 
 * print all processes
 */
int print_proc() {

    struct list_head *pos;
    task_struct *next;

    kernel_printf("----------ALL PROCESSES--------------\n");
    list_for_each(pos, (&all_task)) {
        next = container_of(pos, task_struct, task_list);
        kernel_printf("  PID : %d, name : %s, vruntime : %d\n", next->PID, next->name,
        next->sched_entity.vruntime);
    }
    kernel_printf("----------ALL PROCESSES--------------\n");

    return 0;
}

/* print_rbtree_test : 
 * print red black tree's structure
 */
int print_rbtree_test() {

    /* 
     * disable interrupts to avoid the rb tree being modified 
     * during the printing process
     * it is crucial because we schedule the process in a very short time
     */
    disable_interrupts();
    kernel_printf("-----------------------------------------------------\n");
    kernel_printf("----------CFS structure(Red Black Tree)--------------\n");
    print_process(&(rq.tasks_timeline));
    kernel_printf("----------CFS structure(Red Black Tree)--------------\n");
    kernel_printf("-----------------------------------------------------\n");
    enable_interrupts();
    return 0;

}