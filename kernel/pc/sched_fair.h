#ifndef SCHED_FAIR
#define SCHED_FAIR
#include <zjunix/pc.h>

/*
 * Nice levels are multiplicative, with a gentle 10% change for every
 * nice level changed. I.e. when a CPU-bound task goes from nice 0 to
 * nice 1, it will get ~10% less CPU time than another CPU-bound task
 * that remained on nice 0.
 *
 * The "10% effect" is relative and cumulative: from _any_ nice level,
 * if you go up 1 level, it's -10% CPU usage, if you go down 1 level
 * it's +10% CPU usage. (to achieve that we use a multiplier of 1.25.
 * If a task goes up by ~10% and another task goes down by ~10% then
 * the relative distance between them is ~25%.)
 */
static const int prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};
/*
 * Inverse (2^32/x) values of the prio_to_weight[] array, precalculated.
 *
 * In cases where the weight does not change often, we can use the
 * precalculated inverse to speed up arithmetics by turning divisions
 * into multiplications:
 */
static const int prio_to_wmult[40] = {
 /* -20 */     48388,     59856,     76040,     92818,    118348,
 /* -15 */    147320,    184698,    229616,    287308,    360437,
 /* -10 */    449829,    563644,    704093,    875809,   1099582,
 /*  -5 */   1376151,   1717300,   2157191,   2708050,   3363326,
 /*   0 */   4194304,   5237765,   6557202,   8165337,  10153587,
 /*   5 */  12820798,  15790321,  19976592,  24970740,  31350126,
 /*  10 */  39045157,  49367440,  61356676,  76695844,  95443717,
 /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153,
};


/* CFS-related fields in a runqueue */
struct cfs_rq {
	load_weight load; // total load in queue
	unsigned long nr_running; // total tasks in queue

	unsigned int exec_clock; // cur time clock of cpu
	unsigned int min_vruntime; // min vruntime in queue (leftmost leaf's task)

	struct rb_root tasks_timeline; // cur task
	struct rb_node *rb_leftmost;

	/*
	 * 'curr' points to currently running entity on this cfs_rq.
	 * It is set to NULL otherwise (i.e when none are currently running).
	 */
	struct sched_entity *curr, *next, *last, *skip;

	/*
	 * leaf_cfs_rq_list ties together list of leaf cfs_rq's in a cpu. This
	 * list is used during load balance.
     * defines all tasks on cfs queue, indentical to 'all_ready' defined in pc.c
	 */
	struct list_head leaf_cfs_rq_list;

	/*
	 * the part of load.weight contributed by tasks
	 */
	unsigned long task_weight;

};

struct sched_class {

	struct task_struct * (*pick_next_task) ();
    void (*update_vruntime) ();
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


/*
 * All the scheduling class methods:
 */
struct task_struct * pick_next_task_fair ();

void update_vruntime_fair();

static struct sched_class fair_sched_class;

#endif