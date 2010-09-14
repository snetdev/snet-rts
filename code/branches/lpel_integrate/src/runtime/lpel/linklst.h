#ifndef _LINKLST_H_
#define _LINKLST_H_

#include <stdio.h>


typedef struct list_node list_node_t;
typedef struct list_iter list_iter_t;

typedef struct list_node *list_hnd_t;


extern void ListAppend( list_hnd_t *lst, list_node_t *node);
extern int ListIsEmpty( list_hnd_t *lst);
extern void ListPrint( list_hnd_t *lst, FILE *file);

extern list_node_t *ListNodeCreate( void *data);
extern void ListNodeDestroy( list_node_t *node);
extern void ListNodeSet( list_node_t *node, void *data);
extern void *ListNodeGet( list_node_t *node);



extern list_iter_t *ListIterCreate( list_hnd_t *lst);
extern void ListIterDestroy( list_iter_t *iter);
extern void ListIterReset( list_hnd_t *lst, list_iter_t *iter);
extern int ListIterHasNext( list_iter_t *iter);
extern list_node_t *ListIterNext( list_iter_t *iter);
extern void ListIterAppend( list_iter_t *iter, list_node_t *node);
extern void ListIterRemove( list_iter_t *iter);




#endif /* _LINKLST_H_ */
