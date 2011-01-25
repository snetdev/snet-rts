#ifndef _ASSIGNMENT_H_
#define _ASSIGNMENT_H_

#include "bool.h"
#include "lpel.h"

void AssignmentInit(int lpel_num_workers);
void AssignmentCleanup( void);
int  AssignmentGetWID(lpel_taskreq_t *t, bool is_box, char *boxname);


#endif /* _ASSIGNMENT_H_ */
