#ifndef _ASSIGNMENT_H_
#define _ASSIGNMENT_H_

#include "bool.h"
#include "lpel.h"

void AssignmentInit(int lpel_num_workers);
int  AssignmentGetWID(lpel_task_t *t, bool is_box);


#endif /* _ASSIGNMENT_H_ */
