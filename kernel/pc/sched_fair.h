/*
 *  pc/sched_fair.h
 *
 *  CFS (Complete Fair Scheduler) implementation 
 *
 *  Copyright (C) 2018  Zhenxin Xiao, Zhejiang University
 * 
 */

#ifndef SCHED_FAIR
#define SCHED_FAIR
#include <zjunix/pc.h>
#include <zjunix/rbtree.h>

// the weight value of nice 0 (baseline)
#define NICE_0_LOAD 1024

// if we use the first code, it will cause overflow on hardware
// #define LONG_MAX ((unsigned long)(~0UL>>1))
#define LONG_MAX ((unsigned long)(4294967295 / 2 - 1))
#define min(x, y) (	x > y ? x : y )
#define max(x, y) (	x < y ? x : y )

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// the following prio_to_weight and prio_to_wmult come from Linux Kernel
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
static const long prio_to_weight[40] = {
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
static const long prio_to_wmult[40] = {
 /* -20 */     48388,     59856,     76040,     92818,    118348,
 /* -15 */    147320,    184698,    229616,    287308,    360437,
 /* -10 */    449829,    563644,    704093,    875809,   1099582,
 /*  -5 */   1376151,   1717300,   2157191,   2708050,   3363326,
 /*   0 */   4194304,   5237765,   6557202,   8165337,  10153587,
 /*   5 */  12820798,  15790321,  19976592,  24970740,  31350126,
 /*  10 */  39045157,  49367440,  61356676,  76695844,  95443717,
 /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153,
};
// ------------------------------------------------------------------
// ------------------------------------------------------------------

/* CFS-related fields in a runqueue */
struct cfs_rq {
    /*
	 * the part of load.weight contributed by tasks
	 */
	unsigned long task_weight; // total task's weight in cfs queue
	unsigned long nr_running; // total tasks in queue

	unsigned long exec_clock; // current time clock of cpu
	/* min_vruntime :
	 * min vruntime in queue (leftmost leaf's task)
	 * being used as cache, it can quickly finds out whether this period is over
	 * meaning, the min_vruntime has exceeded LONG_MAX
	 */
	unsigned long min_vruntime;
	/* tasks_timeline :
	 * the root of red black tree
	 */
	struct rb_root tasks_timeline;
	/* rb_leftmost :
	 * the left most rb node of rb tree
	 * being used as cache, it can quickly locate the task that has the 
	 * mininum vruntime in rb tree, which is scheduled to run in the next turn
	 */
	struct rb_node *rb_leftmost;

	/*
	 * 'curr' points to currently running entity on this cfs_rq.
	 * It is set to NULL otherwise (i.e when none are currently running).
	 */
	sched_entity *curr, *next, *last, *skip;

	/*
	 * leaf_cfs_rq_list ties together list of leaf cfs_rq's in a cpu. This
	 * list is used during load balance.
     * defines all tasks on cfs queue, indentical to 'all_ready' defined in pc.c
	 */
	struct list_head leaf_cfs_rq_list;

};

void init_cfs_rq(struct cfs_rq * rq);
void clear_cfs(struct cfs_rq *rq, struct list_head * all_task);

/*
 * All the scheduling class methods:
 */
sched_entity * pick_next_task_fair(struct cfs_rq * rq);
void update_vruntime_fair(struct cfs_rq *cfs_rq, sched_entity *curr, struct list_head * all_task, unsigned long delta_exec);
int insert_process(struct rb_root *root, sched_entity *entity);
void delete_process(struct rb_root *root, sched_entity * entity);
void print_process(struct rb_root *root);


#endif