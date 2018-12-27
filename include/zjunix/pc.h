#ifndef _ZJUNIX_PC_H
#define _ZJUNIX_PC_H

#define TASK_KERNEL_SIZE 4096

#include <zjunix/rbtree.h>
#include <zjunix/list.h>
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

struct sched_entity {
    struct rb_node rb_node;       // rbtree node
    float vruntime;
};

typedef struct {
    struct sched_entity sched_entity;
    context context;
    unsigned int PID;   //pid
    unsigned int parent;   //parent's pid
    unsigned int state;   //state
    char name[32];  //name
    struct list_head sched;
} task_struct;

extern task_struct *current_task;

typedef union {
    task_struct task;
    unsigned char kernel_stack[TASK_KERNEL_SIZE];
} task_union;


void init_pc();
void pc_schedule(unsigned int status, unsigned int cause, context* pt_context);
int pc_peek();
void pc_create(char *task_name, void(*entry)(unsigned int argc, void *args), unsigned int argc, void *args);
void pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context);
int pc_kill(int proc);
task_struct* get_curr_pcb();
int print_proc();
void change_sysctl_sched_latency(unsigned int latency);

#endif  // !_ZJUNIX_PC_H