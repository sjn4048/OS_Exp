#include "sched_fair.h"

void init_CFS(){

    fair_sched_class.pick_next_task	= pick_next_task_fair;
    fair_sched_class.update_vruntime = update_vruntime_fair;

}

// struct task_struct * (*pick_next_task) ();

void update_vruntime (){
};