#ifndef _ASSIGN_H_
#define _ASSIGN_H_

void SNetAssignInit(int lpel_num_workers, int alloc_mode);
void SNetAssignCleanup( void);
int SNetAssignTask(int is_box, const char *boxname);


#endif /* _ASSIGN_H_ */
