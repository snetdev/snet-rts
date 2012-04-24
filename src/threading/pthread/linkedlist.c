#include "linkedlist"
#include "threading.h"

struct snet_linked_list_t {
  snet_stream_desc_t *ds;
  snet_linked_list_t *prev;
  snet_linked_list_t *next;
}

static snet_linked_list_t *SNetLinkedListCreate(snet_stream_desc_t *ds)
{
  snet_linked_list_t *list_ptr = SNetMemAlloc(sizeof(snet_linked_list_t));

  list_ptr->prev = NULL;
  list_ptr->next = NULL;
  list_ptr->ds = ds;
  return list_ptr;
}

snet_linked_list_t *SNetLinkedListAdd(snet_linked_list_t* head,
                                      snet_stream_desc_t *ds)
{
  snet_linked_list_t *list_ptr = SNetLinkedListCreate(ds);
  head->prev = list_ptr;
  list_ptr->next = head;
  return list_ptr;
}

void SNetLinkedListRemove(snet_linked_list_t * list_ptr)
{
  if(list_ptr->prev != NULL) {
    list_ptr->prev->next = list_ptr->next;
  }
  if(list_ptr->next != NULL) {
    list_ptr->next->prev = list_ptr->prev;
  }
  SNetMemFree(list_ptr);
}
