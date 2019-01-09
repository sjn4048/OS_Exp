/*
 *  pc/pc.h
 *
 *  Kernel scheduler and related syscalls
 *
 *  Copyright (C) 2018  Zhenxin Xiao, Zhejiang University
 * 
 */

#ifndef _ZJUNIX_PC_H
#define _ZJUNIX_PC_H

// TASK_KERNEL_SIZE indicates the task structure's kernel size
#define TASK_KERNEL_SIZE 4096

// PC_DEBUG_MODE indicates whether printing debug info or not
// #define PC_DEBUG_MODE
#include <zjunix/fs/fat.h>
#include <zjunix/list.h>
#include <zjunix/rbtree.h>
#include <zjunix/utils.h>
#include "../../usr/ps.h"

/* context : 
 * defines the context registers
 */
typedef struct {
    unsigned int epc;
    unsigned int at;
    unsigned int v0, v1;
    unsigned int a0, a1, a2, a3;
    unsigned int t0, t1, t2, t3, t4, t5, t6, t7;
    unsigned int s0, s1, s2, s3, s4, s5, s6, s7;
    unsigned int t8, t9;
    unsigned int hi, lo;
    unsigned int gp;
    unsigned int sp;
    unsigned int fp;
    unsigned int ra;
} context;

/* load_weight : 
 * the CFS weight and pre-calculated Inverse (2^32/x) values of the weight
 * inv_weight is being used to speed up the calculation
 * by transforming division to multiplication
 * such inspiration also comes from Linux Kernel Code
 */
typedef struct {
	unsigned long weight, inv_weight;  // task's weight 
} load_weight;

/* sched_entity : 
 * this structure defines the CFS schedule class
 * it mainly stores the running info about this task
 * And most importantly, the red black tree's node of this task
 */
typedef struct {

    struct rb_node rb_node;       // rbtree node
    unsigned long vruntime;       // vruntime of cfs
    load_weight load;		/* for load-balancing */
	unsigned long exec_start_time;    // task's start time of this period
	unsigned long sum_exec_runtime;  // total run time

}sched_entity;

/* task_struct : 
 * this structure mainly defines the basic info about this task
 */
typedef struct {
    int nice;    // nice value of this task

    /* static_prio, normal_prio : 
     * static_prio is the prioity defined when the task is created and will 
     * never change again
     * however, normal_prio will change due to the task's behavior( I/O task 
     * or compute-intensive task), normally implied by 'usage' defined below
     */
    int static_prio, normal_prio;

    sched_entity sched_entity; // schedule entity
    context context; // context register

    unsigned int PID;   //pid
    unsigned int parent;   //parent's pid
    unsigned int state;   //state
    char name[32];  //name

    struct list_head task_list; // task pointer
    struct list_head state_list; // state pointer

    /* usage : 
	 * record the cpu usage of this task
     * being used to imply it is a I/O task or compute-intensive task
	 */
    unsigned int usage; 

    /* children and children_head: 
	 * a list contains all chrildren of this task
     * when this task is terminated
     * kill all children of this task recursivelly
	 */
    struct list_head children_head;
    struct list_head children;

} task_struct;

// current running task pointer
extern task_struct *current_task;

/* task_union : 
 * this union makes sure that the task_struct's size is forced to be 4096 bits
 * we do this because we store the registers in the end of the task_struct
 * see more info in 'start.s'
 */
typedef union {
    task_struct task;
    unsigned char kernel_stack[TASK_KERNEL_SIZE];
} task_union;


void init_pc();
void pc_schedule(unsigned int status, unsigned int cause, context* pt_context);
void pc_create(char *task_name, void(*entry)(unsigned int argc, char *args), 
    unsigned int argc, char *args, int nice, int is_root, int need_wait);
unsigned int pc_create_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context);
int pc_kill(unsigned int PID);
int pc_exit(context* pt_context);
int print_proc();
char * task_state(int state);
int print_rbtree_test();
void change_sysctl_sched_latency(unsigned int latency);
extern void *kmalloc(unsigned int size);
extern void restore_context();
extern unsigned int GetPC();
int exec_from_file(char* filename);
int pc_kill_current();
void kill_all_children(struct list_head * head);
void test_program(unsigned int argc, char *args);
unsigned int atoi(char * param);
#endif  // !_ZJUNIX_PC_H