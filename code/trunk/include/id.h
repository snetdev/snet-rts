#ifndef _SNET_ID_H_
#define _SNET_ID_H_

#include <stdint.h>

#define SNET_ID_NO_ID 0

#define SNET_ID_MPI_TYPE MPI_LONG_LONG

typedef uint64_t snet_id_t;

#define SNET_ID_CREATE(node, id) ((((uint64_t) id) & 0xFFFF) | (((uint64_t)node) << 0x20))
#define SNET_ID_GET_ID(id)       (unsigned int)((uint64_t) id & 0xFFFF) 
#define SNET_ID_GET_NODE(id)     (unsigned int)((((uint64_t)id) >> 0x20)& 0xFFFF)

void SNetIDServiceInit(int id);

void SNetIDServiceDestroy();

snet_id_t SNetIDServiceGetID();

int SNetIDServiceGetNodeID();

#endif /* _SNET_ID_H_ */
