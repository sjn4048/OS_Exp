#include "sched_fair.h"


// struct task_struct * (*pick_next_task) ();

void update_vruntime_fair(){

};

void init_cfs_rq(struct cfs_rq * rq){
    rq->task_weight = 0;
	rq->nr_running = 0;

	rq->exec_clock = 0;
	rq->min_vruntime = 0;
	rq->tasks_timeline = RB_ROOT;
	rq->rb_leftmost = 0;
	struct sched_entity *curr = 0, *next = 0, *last = 0, *skip = 0;
    INIT_LIST_HEAD(&(rq->leaf_cfs_rq_list));
}

struct rb_node * pick_next_task_fair(struct cfs_rq * rq){
    return rb_entry(rq->rb_leftmost, struct sched_entity, rb_node);
}