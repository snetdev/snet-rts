#ifndef factorial_h
#define factorial_h


#include <snetentities.h>


#define F_x     0
#define F_r     1
#define F_xx    2
#define F_rr    3
#define F_one   4
#define F_p     5

#define T_T     6
#define T_F     7
#define T_out   8
#define T_ofl   9
#define T_stop 10
#define F_z    11
#define T_s    12
#define BT_tst 13 
#define NUM_NAMES 14
#define MAX_NAME_LEN 10



snet_buffer_t *SER_starnet_filter( snet_buffer_t *inbuf);

#endif
