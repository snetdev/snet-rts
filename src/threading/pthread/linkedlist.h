#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

#include "threading.h"

typedef struct snet_linked_list_t snet_linked_list_t;

snet_linked_list_t *SNetLinkedListAdd(snet_linked_list_t *head,
                                      snet_stream_desc_t *ds);
void SNetLinkedListRemove(snet_linked_list_t *list_ptr);

#endif
