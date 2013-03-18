#ifndef _ASSIGN_H_
#define _ASSIGN_H_

typedef enum  {
    ASSIGN_ONE_THREAD, /* all on one worker */
    ASSIGN_ROUND_ROBIN_SHARED, /* round-robin on all entities, boxes switch to next worker */
    ASSIGN_ROUND_ROBIN_PER_ENTITY, /* round-robin per box + global round-robin for other entities */
    ASSIGN_D_SNET_EXPLICIT, /* explicit placement operator in D-SNET */
} default_task_assignment_t;

extern default_task_assignment_t default_placement;

void SNetAssignInit(int lpel_num_workers);
void SNetAssignCleanup( void);
int SNetAssignTask(int is_box, const char *boxname);


#endif /* _ASSIGN_H_ */
