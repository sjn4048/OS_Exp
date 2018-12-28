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
	unsigned long weight, inv_weight;
} load_weight;

struct sched_entity {
    struct rb_node rb_node;       // rbtree node
    unsigned int vruntime;
    load_weight load;		/* for load-balancing */
	unsigned int exec_start;
	unsigned int sum_exec_runtime;
	unsigned int prev_sum_exec_runtime;
	unsigned int nr_migrations;
};

typedef struct {
    struct sched_entity sched_entity;
    context context;
    unsigned int PID;   //pid
    unsigned int parent;   //parent's pid
    unsigned int state;   //state
    char name[32];  //name
    struct list_head task;
    unsigned int usage;
    int prio, static_prio, normal_prio;
    const struct sched_class *sched_class;
    struct list_head children;
} task_struct;

extern task_struct *current_task;

typedef union {
    task_struct task;
    unsigned char kernel_stack[TASK_KERNEL_SIZE];
} task_union;

/* CFS-related fields in a runqueue */
struct cfs_rq {
	load_weight load;
	unsigned long nr_running;

	unsigned int exec_clock;
	unsigned int min_vruntime;

	struct rb_root tasks_timeline;
	struct rb_node *rb_leftmost;

	struct list_head tasks;
	struct list_head *balance_iterator;

	/*
	 * 'curr' points to currently running entity on this cfs_rq.
	 * It is set to NULL otherwise (i.e when none are currently running).
	 */
	struct sched_entity *curr, *next, *last, *skip;

	unsigned int nr_spread_over;

	struct rq *rq;	/* cpu runqueue to which this cfs_rq is attached */

	/*
	 * leaf cfs_rqs are those that hold tasks (lowest schedulable entity in
	 * a hierarchy). Non-leaf lrqs hold other higher schedulable entities
	 * (like users, containers etc.)
	 *
	 * leaf_cfs_rq_list ties together list of leaf cfs_rq's in a cpu. This
	 * list is used during load balance.
	 */
	int on_list;
	struct list_head leaf_cfs_rq_list;
	struct task_group *tg;	/* group that "owns" this runqueue */

	/*
	 * the part of load.weight contributed by tasks
	 */
	unsigned long task_weight;

};

struct sched_class {

	struct task_struct * (*pick_next_task) (struct rq *rq);

	// void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
	// void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);
	// void (*yield_task) (struct rq *rq);

	// void (*check_preempt_curr) (struct rq *rq, struct task_struct *p, int flags);

	// void (*put_prev_task) (struct rq *rq, struct task_struct *p);

	// void (*set_curr_task) (struct rq *rq);
	// void (*task_tick) (struct rq *rq, struct task_struct *p, int queued);
	// void (*task_fork) (struct task_struct *p);

	// void (*switched_from) (struct rq *this_rq, struct task_struct *task);
	// void (*switched_to) (struct rq *this_rq, struct task_struct *task);
	// void (*prio_changed) (struct rq *this_rq, struct task_struct *task, int oldprio);

	// unsigned int (*get_rr_interval) (struct rq *rq, struct task_struct *task);

};

void init_pc();
void pc_schedule(unsigned int status, unsigned int cause, context* pt_context);
void pc_create(char *task_name, void(*entry)(unsigned int argc, void *args), unsigned int argc, void *args);
void pc_kill_syscall(unsigned int status, unsigned int cause, context* pt_context);
int pc_kill(unsigned int PID);
int print_proc();
void change_sysctl_sched_latency(unsigned int latency);
extern void *kmalloc(unsigned int size);

#endif  // !_ZJUNIX_PC_H