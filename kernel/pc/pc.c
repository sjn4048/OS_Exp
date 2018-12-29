#include "pc.h"
#include <driver/vga.h>
#include <intr.h>
#include <arch.h>
#include <zjunix/syscall.h>
#include <zjunix/log.h>

#define DEBUG_MODE
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
void add_task(task_struct *task, struct list_head * tasks) {
    list_add_tail(&(task->task_list), tasks);
}
void remove_task(task_struct *task) {
    list_del(&(task->task_list));
    INIT_LIST_HEAD(&(task->task_list));
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
    // ---------------------------------------------------
    // ---------- setting idle task ----------------------
    task_union *new = (task_union*)(kernel_sp - TASK_KERNEL_SIZE);
    task_struct * task = &(new->task);
    task->PID = cur_PID++;
    task->parent = -1;
    task->state = TASK_RUNNING;
    INIT_LIST_HEAD(&(task->task_list));
    INIT_LIST_HEAD(&(task->children));
    kernel_strcpy(task->name, "idle");
    task->nice = 0; // default nice value
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
    add_task(&(new->task), &all_task);
    insert_process(&(rq.tasks_timeline),&(task->sched_entity));
    // ---------- done setting idle task -----------------
    // ---------------------------------------------------
    register_syscall(10, pc_kill_syscall);
    register_interrupt_handler(7, pc_schedule);
    // asm volatile(   // huge bug here, piece of shit...
    //     //"li $v0, %0\n\t"
    //     "mtc0 $0, $11\n\t"
    //     "mtc0 $zero, $9"
    //     : : "r"(sysctl_sched_latency));
    current_task = &(new->task);
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

    disable_interrupts();
    update_vruntime_fair(&(rq),&(current_task->sched_entity),1);
    if (rq.min_vruntime >= LONG_MAX - 1){
		clear_cfs(&(rq), &(all_task));
	}
    enable_interrupts();

    sched_entity *entity = pick_next_task_fair(&(rq));
    task_struct * next = container_of(entity, task_struct, sched_entity);
    copy_context(pt_context, &(current_task->context));
    copy_context(&(next->context), pt_context);
    current_task->state = TASK_READY;
    next->state = TASK_RUNNING;
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
    sched_entity* entity = &(task->sched_entity);
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
    add_task(task, &all_task);
    insert_process(&(rq.tasks_timeline),&(task->sched_entity));
    if (task_name[0] == 'p')
        other = task;
}

void pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context) {

    // pc_kill(current_task);
    // pc_schedule(status, cause, pt_context);
    
}

// kill a process by PID
int pc_kill(unsigned int PID) {
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
    // find that task
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
    remove_task(task);
    delete_process(&(rq.tasks_timeline), &(task->sched_entity));

    enable_interrupts();
    return 0;
}


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

    #ifdef DEBUG_MODE
        kernel_printf("----------CFS structure(Red Black Tree)--------------\n");
        print_process(&(rq.tasks_timeline));
        kernel_printf("----------CFS structure(Red Black Tree)--------------\n");
    #endif

    return 0;
}
