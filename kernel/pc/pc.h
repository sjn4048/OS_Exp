#ifndef _PC_H
#define _PC_H

#include <zjunix/pc.h>
#include "sched_fair.h"

struct sched_class {

	struct task_struct * (*pick_next_task) (struct cfs_rq *rq);

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

#endif
