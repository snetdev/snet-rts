#ifndef graphicFactorial_h
#define graphicFactorial_h

#include <snetentities.h>
#include <buffer.h>


#define F_x     0
#define F_r     1
#define F_xx     2
#define F_rr     3
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


extern snet_buffer_t *SER_net_graph( snet_buffer_t *inbuf);

#endif
