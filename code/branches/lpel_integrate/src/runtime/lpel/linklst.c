/*
 * Implements a linked set as circular single linked list.
 *
 */

#include <stdlib.h>
#include <assert.h>

#include "linklst.h"



struct list_node {
  struct list_node *next;
  void *data;
};


struct list_iter {
  list_node_t *cur;
  list_node_t *prev;
  list_hnd_t  *list;
};


/**
 * it is safe to append while iterating
 */
void ListAppend( list_hnd_t *lst, list_node_t *node)
{
  if (*lst  == NULL) {
    /* list is empty */
    *lst = node;
    node->next = node; /* selfloop */
  } else { 
    /* insert stream between last element=*lst and first element=(*lst)->next */
    node->next = (*lst)->next;
    (*lst)->next = node;
    *lst = node;
  }
}


/**
 * 
 */
int ListIsEmpty( list_hnd_t *lst)
{
  return (*lst == NULL);
}

/**
 * @pre   file is opened for writing
 */
void ListPrint( list_hnd_t *lst, FILE *file)
{
  list_node_t *cur;

  //fprintf( file,"--TAB--\n" );
  fprintf( file,"[ " );

  if (*lst != NULL) {
    cur = (*lst)->next;
    do {
      fprintf( file, "%p ", cur->data);
      //fprintf( file, "(%p: %p)", cur, cur->data);
      cur = cur->next;
    } while (cur != (*lst)->next);
  }

  // fprintf( file,"-------\n" );
  fprintf( file,"]\n" );

}


list_node_t *ListNodeCreate( void *data)
{
  list_node_t *node;
  node = (list_node_t *) malloc( sizeof( list_node_t));
  node->data = data;
  return node;
}

void ListNodeDestroy( list_node_t *node)
{
  free( node);
}

void ListNodeSet( list_node_t *node, void *data)
{
  node->data = data;  
}

void *ListNodeGet( list_node_t *node)
{
  return node->data;
}



/**
 *
 */
list_iter_t *ListIterCreate( list_hnd_t *lst)
{
  list_iter_t *iter = (list_iter_t *) malloc( sizeof(list_iter_t));
  if (lst) {
    iter->prev = *lst;
    iter->list = lst;
  }
  iter->cur = NULL;
  return iter;
}

void ListIterDestroy( list_iter_t *iter)
{
  free(iter);
}

void ListIterReset( list_hnd_t *lst, list_iter_t *iter)
{
  iter->prev = *lst;
  iter->list = lst;
  iter->cur = NULL;
}


int ListIterHasNext( list_iter_t *iter)
{
  return (*iter->list != NULL) &&
    ( (iter->cur != *iter->list) || (iter->cur == NULL) );
}


list_node_t *ListIterNext( list_iter_t *iter)
{
  assert( ListIterHasNext(iter) );

  if (iter->cur != NULL) {
    /* this also does account for the state after deleting */
    iter->prev = iter->cur;
    iter->cur = iter->cur->next;
  } else {
    iter->cur = iter->prev->next;
  }
  return iter->cur;
}


void ListIterAppend( list_iter_t *iter, list_node_t *node)
{
  /* insert after cur */
  node->next = iter->cur->next;
  iter->cur->next = node;
  /* if current node was last node, update list */
  if (iter->cur == *iter->list) {
    *iter->list = node;
  }

  /* handle case if current was single element */
  if (iter->prev == iter->cur) {
    iter->prev = node;
  }
}

/**
 * Remove the current node from list
 * @pre iter points to valid element
 * @note IterRemove() may only be called once after
 *       IterNext(), after a  
 */
void ListIterRemove( list_iter_t *iter)
{
  /* handle case if there is only a single element */
  if (iter->prev == iter->cur) {
    assert( iter->prev == *iter->list );
    iter->cur->next = NULL;
    *iter->list = NULL;
  } else {
    /* remove cur */
    iter->prev->next = iter->cur->next;
    /* cur becomes invalid */
    iter->cur->next = NULL;
    /* if the first element was deleted, clear cur */
    if (*iter->list == iter->prev) {
      iter->cur = NULL;
    } else {
      /* if the last element was deleted, correct list */
      if (*iter->list == iter->cur) {
        *iter->list = iter->prev;
      }
      iter->cur = iter->prev;
    }
  }
}

