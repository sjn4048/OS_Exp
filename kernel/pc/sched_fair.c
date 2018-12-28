#include "sched_fair.h"
#include <driver/vga.h>

// struct task_struct * (*pick_next_task) ();

void update_vruntime_fair(task_struct *current_task){
    // current_task->sched_entity.vruntime += current_task->sched_entity.load.weight;
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

struct sched_entity * pick_next_task_fair(struct cfs_rq * rq){
    return rb_entry(rq->rb_leftmost, struct sched_entity, rb_node);
}


/*
 * insert a node into rbtree
 */
int insert_process(struct rb_root *root, struct sched_entity *entity)
{
	unsigned long vruntime = entity->vruntime;
    struct rb_node **tmp = &(root->rb_node), *parent = Null;

    /* Figure out where to put new node */
    while (*tmp)
    {
        struct sched_entity *entity = container_of(*tmp, struct sched_entity, rb_node);

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

void delete_process(struct rb_root *root, struct sched_entity * entity)
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
		struct sched_entity * entity = rb_entry(tree, struct sched_entity, rb_node);
		task_struct *task = container_of(entity, task_struct, sched_entity);
        if (direction==0)    // tree is root
            kernel_printf("  %s(B) is root\n", task->name);
        else{                // tree is not root
			struct sched_entity * parent_entity = rb_entry(parent, struct sched_entity, rb_node);
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

