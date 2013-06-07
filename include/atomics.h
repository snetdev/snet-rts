#ifndef _ATOMICS_H_
#define _ATOMICS_H_

#define CAS(ptr, old, val)      __sync_bool_compare_and_swap(ptr, old, val)
#define AAF(ptr, val)           __sync_add_and_fetch(ptr, val)
#define FAA(ptr, val)           __sync_fetch_and_add(ptr, val)
#define SAF(ptr, val)           __sync_sub_and_fetch(ptr, val)
#define FAS(ptr, val)           __sync_fetch_and_sub(ptr, val)
#define BAR()                   __sync_synchronize()

#endif /* _ATOMICS_H_ */

