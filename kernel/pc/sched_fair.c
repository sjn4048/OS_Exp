#include "sched_fair.h"

void init_CFS(){

    fair_sched_class.pick_next_task	= pick_next_task_fair;

}

