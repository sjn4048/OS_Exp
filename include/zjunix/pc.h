#ifndef _ZJUNIX_PC_H
#define _ZJUNIX_PC_H

#define TASK_KERNEL_SIZE 4096

#include <zjunix/list.h>
#include <zjunix/rbtree.h>
#include <zjunix/utils.h>



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

typedef struct {
	unsigned long weight, inv_weight;  // task's weight 
} load_weight;

struct sched_entity {
    struct rb_node rb_node;       // rbtree node
    unsigned long vruntime;       // vruntime of cfs
    load_weight load;		/* for load-balancing */
	unsigned long exec_start_time;    // task's start time of this period
	unsigned long sum_exec_runtime;  // total run time

};

typedef struct {
    int nice;    // nice value of this task
    int static_prio, normal_prio;
    struct sched_entity sched_entity; // schedule entity
    context context; // context register
    unsigned int PID;   //pid
    unsigned int parent;   //parent's pid
    unsigned int state;   //state
    char name[32];  //name
    struct list_head task_list; // task pointer
    /* usage : 
	 * record the cpu usage of this task
     * being used to imply it is a I/O task or compute-intensive task
	 */
    unsigned int usage; 
    /* children : 
	 * a list contains all chrildren of this task
     * when this task is terminated
     * kill all children of this task recursivelly
	 */
    struct list_head children;
} task_struct;

// current running task
extern task_struct *current_task;

typedef union {
    task_struct task;
    unsigned char kernel_stack[TASK_KERNEL_SIZE];
} task_union;



void init_pc();
void pc_schedule(unsigned int status, unsigned int cause, context* pt_context);
void pc_create(char *task_name, void(*entry)(unsigned int argc, void *args), unsigned int argc, void *args, int nice);
void pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context);
int pc_kill(unsigned int PID);
int print_proc();
void change_sysctl_sched_latency(unsigned int latency);
extern void *kmalloc(unsigned int size);

#endif  // !_ZJUNIX_PC_H