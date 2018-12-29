#include "sched_fair.h"
#include <driver/vga.h>



unsigned long calc_delta_mine(unsigned long delta_exec, load_weight *lw){
	unsigned long tmp;

	if (!lw->inv_weight) {
		lw->inv_weight = NICE_0_LOAD;
	}
	/*
	* Note : ----------------------------------------------------------
	* This is slightly different from the original linux implementation
	* where tmp is also mulipulated by 1024(nice 0's load)
	* In order to prevent overflow on 32-bit machine(ZJUNIX)
	* and does not lose precision, we delelop the following formula :
	* Note : ----------------------------------------------------------
	*/
	tmp = (unsigned long)delta_exec * lw->inv_weight;

	return (unsigned long)min(tmp, LONG_MAX);
}

unsigned long calc_delta_fair(unsigned long delta, sched_entity *se)
{
	if (se->load.weight != NICE_0_LOAD)
		delta = calc_delta_mine(delta, &(se->load));

	return delta;
}

static void update_min_vruntime(struct cfs_rq *rq)
{
	unsigned long vruntime = rq->min_vruntime;

	if (rq->rb_leftmost) {
		sched_entity *se = rb_entry(rq->rb_leftmost, sched_entity, rb_node);
		vruntime = se->vruntime;
	}

	rq->min_vruntime = max(rq->min_vruntime, vruntime);
}

struct rb_node * find_rb_leftmost(struct cfs_rq *rq){
	struct rb_node *tree = rq->tasks_timeline.rb_node;
	while(tree->rb_left != Null)
    {
        if (!(tree->rb_left))
			break;
    }

	if (tree->rb_right){
		return tree->rb_right;
	}
	else{
		return tree;
	}
};

void clear_cfs(struct cfs_rq *rq, struct list_head * all_task){
	
	struct list_head *pos;
    task_struct *next;
	kernel_printf("-----------------clear_cfs-----------------");
    list_for_each(pos, all_task) {
        next = container_of(pos, task_struct, task_list);
        next->sched_entity.vruntime = 0;
		delete_process(&(rq->tasks_timeline), &next->sched_entity);
    }

	list_for_each(pos, all_task) {
        next = container_of(pos, task_struct, task_list);
		insert_process(&(rq->tasks_timeline), &next->sched_entity);
    }

	rq->rb_leftmost = find_rb_leftmost(rq);
	rq->min_vruntime = 0;
	kernel_printf("-----------------done clear_cfs------------");
}
void update_vruntime_fair(struct cfs_rq *rq, sched_entity *curr,
	      unsigned long delta_exec)
{
	unsigned long delta_exec_weighted;
kernel_printf("0\n");
	curr->sum_exec_runtime += delta_exec;
	delta_exec_weighted = calc_delta_fair(delta_exec, curr);
kernel_printf("1\n");
	curr->vruntime += delta_exec_weighted;
	// rebalance the rb tree
	delete_process(&(rq->tasks_timeline), curr);
kernel_printf("2\n");
	insert_process(&(rq->tasks_timeline), curr);
kernel_printf("3\n");
	// update leftmost task
	rq->rb_leftmost = find_rb_leftmost(rq);
kernel_printf("4\n");
	// update min vruntime of CFS queue
	update_min_vruntime(rq);
kernel_printf("5\n");
}


void init_cfs_rq(struct cfs_rq * rq){
    rq->task_weight = 0;
	rq->nr_running = 0;

	rq->exec_clock = 0;
	rq->min_vruntime = 0;
	rq->tasks_timeline = RB_ROOT;
	rq->rb_leftmost = 0;
	sched_entity *curr = 0, *next = 0, *last = 0, *skip = 0;
    INIT_LIST_HEAD(&(rq->leaf_cfs_rq_list));
}

sched_entity * pick_next_task_fair(struct cfs_rq * rq){
    return rb_entry(rq->rb_leftmost, sched_entity, rb_node);
}


/*
 * insert a node into rbtree
 */
int insert_process(struct rb_root *root, sched_entity *entity)
{
	unsigned long vruntime = entity->vruntime;
    struct rb_node **tmp = &(root->rb_node), *parent = Null;

    /* Figure out where to put new node */
    while (*tmp)
    {
        sched_entity *entity = container_of(*tmp, sched_entity, rb_node);

        parent = *tmp;
        if (vruntime <= entity->vruntime)
            tmp = &((*tmp)->rb_left);
        else if (vruntime > entity->vruntime)
            tmp = &((*tmp)->rb_right);
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&(entity->rb_node), parent, tmp);
    rb_insert_color(&(entity->rb_node), root);

    return 0;
}

void delete_process(struct rb_root *root, sched_entity * entity)
{
    // delete process
    rb_erase(&(entity->rb_node), root);
    
}

/*
 * DEBUG : print rbtree
 */
void print_rbtree(struct rb_node *tree, struct rb_node *parent, int direction)
{
    if(tree != Null)
    {
		sched_entity * entity = rb_entry(tree, sched_entity, rb_node);
		task_struct *task = container_of(entity, task_struct, sched_entity);
        if (direction==0)    // tree is root
            kernel_printf("  %s(B) is root\n", task->name);
        else{                // tree is not root
			sched_entity * parent_entity = rb_entry(parent, sched_entity, rb_node);
			task_struct * parent_task = container_of(parent_entity, task_struct, sched_entity);
            kernel_printf("  %s(%s) is %s's %s child\n", task->name, rb_is_black(tree)?"B":"R", parent_task->name, direction==1?"right" : "left");
		}
        if (tree->rb_left)
            print_rbtree(tree->rb_left, tree, -1);
        if (tree->rb_right)
            print_rbtree(tree->rb_right, tree, 1);
    }
}

/*
 * DEBUG : print rbtree
 */
void print_process(struct rb_root *root)
{
    if (root!=Null && root->rb_node!=Null)
        print_rbtree(root->rb_node, 0, 0);
}

