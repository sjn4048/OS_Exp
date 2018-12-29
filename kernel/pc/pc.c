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

// append a task into list
#define add_task(task, tasks, pointer) \
            (list_add_tail(&((task)->pointer), (tasks)))

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
    INIT_LIST_HEAD(&(task->state_list));
    INIT_LIST_HEAD(&(task->children));
    INIT_LIST_HEAD(&(task->children_head));

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

    add_task(&(new->task), &all_task, task_list);
    add_task(&(new->task), &all_ready, state_list);
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
    update_vruntime_fair(&(rq),&(current_task->sched_entity),&(all_ready),1);

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

/* hangup_parent : 
 * hang up parent process, waiting the child completes
 */
void hangup_parent(task_struct * parent){

    remove_task(parent,state_list);
    add_task(parent,&all_waiting,state_list);
    parent->state = TASK_WAITING;
    delete_process(&(rq.tasks_timeline),&(parent->sched_entity));

}

/* wakeup_parent : 
 * wake up parent process, after the child completes
 */
void wakeup_parent(unsigned long pid){
    struct list_head *pos;
    task_struct *next;

    int find_flag = 0;
    list_for_each(pos, (&all_waiting)){
        next = container_of(pos, task_struct, state_list);
        if (next->PID == pid){
            find_flag = 1;
            break;
        }
    }
    if (find_flag == 0){
        // NOTE : not a bug, meaning the parent is not waiting the child
        // just return and do nothing 
        return;
    }

    remove_task(next,state_list);
    add_task(next,&all_ready,state_list);
    next->state = TASK_READY;
    insert_process(&(rq.tasks_timeline),&(next->sched_entity));

}

/* pc_create : 
 * create a process 
 */
void pc_create(char *task_name, void(*entry)(unsigned int argc, void *args), unsigned int argc, void *args, int nice, int is_root, int need_wait) {

    task_union *new = (task_union*) kmalloc(sizeof(task_union));
    task_struct * task = &(new->task);
    kernel_strcpy(task->name, task_name);
    INIT_LIST_HEAD(&(task->task_list));
    INIT_LIST_HEAD(&(task->state_list));
    INIT_LIST_HEAD(&(task->children));
    INIT_LIST_HEAD(&(task->children_head));

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
    if (is_root == 0){
        task->parent = current_task->PID;
    }else{
        task->parent = -1;
    }
    
    if (is_root == 0 && need_wait == 1){
        hangup_parent(current_task);
    }

    // if this task is not root task
    // add new task to parents' children_head list
    if (is_root == 0){
        list_add_tail(&(task->children), &(current_task->children_head));
    }

    task->state = TASK_READY;

    // add to coresponding task queue(s)
    add_task(task, &all_task, task_list);
    add_task(task, &all_ready, state_list);
    insert_process(&(rq.tasks_timeline),&(task->sched_entity));

    // update leftmost task
	rq.rb_leftmost = find_rb_leftmost(&(rq));
	// update min vruntime of CFS queue
	update_min_vruntime(&(rq));

}

/* check_if_ps_exit :
 * if powershell is exiting, create a new powershell process
 * to prevent OS from dead, this is quite useful when a unhandled
 * exception occurs, it keeps the OS running almost forever!!!
 */
void check_if_ps_exit(){
    
    if (!kernel_strcmp(current_task->name, "powershell")){
        kernel_printf("----------PowerShell Process exiting-------------\n");
        kernel_printf("----------Recreating PowerShell Process----------\n");
        pc_create("powershell",(void*)ps,0,0,1,1,0);
        kernel_printf("----------PowerShell Process created----------\n");
    }

}

/* pc_kill_syscall : 
 * process kill system call, it can be used to forcely terminate 
 * a process when a unhandled exception occurs
 * see more info in 'exc.c'
 */
void pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context) {
    
    disable_interrupts();
    check_if_ps_exit();
    pc_exit(pt_context);
    // pc_schedule(status, cause, pt_context);
    enable_interrupts();
}

int pc_kill_current(){
    /* stable implementation :
     * use syscall to call "pc_kill_syscall" function and kill the 
     * current process. 
     */
    asm volatile(
        "li $v0, 10\n\t"
        "syscall\n\t"
        "nop\n\t");
    
    /* Another implementation : 
     * ( NOTE : still some unknown bugs, maybe unstatble!!! )
     * in order to get 'pt_context' info which can't be accessed directly 
     * if it's not exception or interrupts, we use inline assemble code to
     * get "sp" register, save context registers to it and call restore_context
     * function in 'start.s'
     */
    // disable_interrupts();
    // check_if_ps_exit();
    // unsigned int sp = 0;
    // asm volatile(   "move %0, $sp\n\t"
    //                 "addi $sp, $sp, -32\n\t"
    //                 : "=r"(sp)); 
    // pc_exit((context * )sp);
    // asm volatile(   "nop\n\t"
	//                 "addi $sp, $sp, 32"); 
    // restore_context();
    // enable_interrupts();

    return 0;
}

/* pc_exit : 
 * kill itself (current running process)
 * and switch register context to load a new process
 */
int pc_exit(context* pt_context){

    // idle task is dead
    if (current_task->PID == 0) {
        kernel_printf("IDLE PROCESS IS EXITTING!\n");
        while(1)
        ;
    }

    task_struct *task = current_task;
    task->state = TASK_DEAD;

    // clean up the task queue
    remove_task(task, state_list);
    add_task(task, &all_dead, state_list);
    delete_process(&(rq.tasks_timeline), &(task->sched_entity));

    // check if this task has belongs to any father
    // if true, remove it from it's father's children list
    if (!list_empty(&(task->children))){
        remove_task(task,children);
    }

    // check if this task has any children 
    // if true, recursively kill all children of it
    if (!list_empty(&(task->children_head))){
        kill_all_children(&(task->children_head));
    }

    // check if parent is waiting
    wakeup_parent(task->parent);

    // update leftmost task
	rq.rb_leftmost = find_rb_leftmost(&(rq));
	// update min vruntime of CFS queue
	update_min_vruntime((&(rq)));

    // find a process to run
    sched_entity *entity = pick_next_task_fair(&(rq));
    current_task = container_of(entity, task_struct, sched_entity);

    // switch context registers
    copy_context(&(current_task->context),pt_context);

    return 0;
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
    list_for_each(pos, (&all_ready)) {
        task = container_of(pos, task_struct, state_list);
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
    remove_task(task, state_list);
    add_task(task, &all_dead, state_list);
    delete_process(&(rq.tasks_timeline), &(task->sched_entity));

    // check if this task has belongs to any father
    // if true, remove it from it's father's children list
    if (!list_empty(&(task->children))){
        remove_task(task,children);
    }

    // check if this task has any children 
    // if true, recursively kill all children of it
    if (!list_empty(&(task->children_head))){
        kill_all_children(&(task->children_head));
    }

    // check if parent is waiting
    wakeup_parent(task->parent);

    // update leftmost task
	rq.rb_leftmost = find_rb_leftmost(&(rq));
	// update min vruntime of CFS queue
	update_min_vruntime(&(rq));

    enable_interrupts();
    return 0;

}

/* kill_all_children : 
 * kill all children's tasks when the father task exits
 */
void kill_all_children(struct list_head * head){
    struct list_head *pos;
    task_struct *next;
    list_for_each(pos, head){
        next = container_of(pos, task_struct, children);
        /* NOTE : 
         * we need to check the process we are killing is running right
         * now or not, this is crucial because if we kill the current 
         * running process by "pc_kill", the OS will be dead
         */
        if (next == current_task){
            pc_kill_current();
        }else{
            pc_kill(next->PID);
        }
    }
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
        kernel_printf("  PID : %d, name : %s, state : %s, vruntime : %d\n", next->PID, next->name, task_state(next->state),
        next->sched_entity.vruntime);
    }
    kernel_printf("----------ALL PROCESSES--------------\n");

    return 0;
}

/* task_state : 
 * return task state in string
 */
char * task_state(int state){
    if (state == TASK_RUNNING){
        return "TASK_RUNNING";
    }
    else if (state == TASK_READY){
        return "TASK_READY";
    }
    else if (state == TASK_WAITING){
        return "TASK_WAITING";
    }
    else{
        return "TASK_DEAD" ;
    }
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

/* exec_from_file : 
 * loading .bin file from disk and run it
 * currently only run it in kernel mode
 */
int exec_from_file(char* filename) {

    FILE file;
    const unsigned int CACHE_BLOCK_SIZE = 64;
    unsigned char buffer[512];
    int result = fs_open(&file, filename);
    if (result != 0) {
        kernel_printf("File %s not exist\n", filename);
        return 1;
    }
    unsigned int size = get_entry_filesize(file.entry.data);
    unsigned int n = size / CACHE_BLOCK_SIZE + 1;
    unsigned int i = 0;
    unsigned int j = 0;
    kernel_printf("%d\n", size);
    unsigned int ENTRY = (unsigned int)kmalloc(4096);
    for (j = 0; j < n; j++) {
        fs_read(&file, buffer, CACHE_BLOCK_SIZE);
        kernel_memcpy((void*)(ENTRY + j * CACHE_BLOCK_SIZE), buffer, CACHE_BLOCK_SIZE);
        kernel_cache(ENTRY + j * CACHE_BLOCK_SIZE);
    }
    int (*f)() = (int (*)())(ENTRY);
    int ret = f();
    // pc_create("seg",(void *)ENTRY,0,0,0,1);
    // kfree((void*)ENTRY);
    return ret;
    
}