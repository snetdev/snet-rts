#include "tree.h"
#include "bool.h"
#include "debug.h"
#include "stack.h"
#include "list.h"
struct tree {
  snet_util_list_t *children;
  bool content_defined;
  void *content;
};

struct numerated_child {
  int index;
  struct tree *child;
};

/**
 *
 * @file tree.c
 *
 *    This implements a tree that maps snet_util_stacks that contain 
 *    integer pointers to some elements. 
 *
 *    Design decision: The topmost element in the stack is in the lowest 
 *    element in the tree, the bottom-most element in the stack is in the
 *    root-element in the tree.
 *
 *    Design decision: the childlists are sorted by insertion. We will search
 *    the right place from behind, as we can assume the elements of the
 *    stack to grow monotonically if new children need to be created.
 */

/**<!--**********************************************************************-->
 *
 * @fn struct tree *traverse(snet_util_tree_t *tree, snet_util_stack_t *key)
 *
 * @brief consumes the stack and traverses the tree accordingly. 
 *
 *      This consumes the stack and traverses the tree accordingly. If we
 *      run into missing children or similar things, this returns NULL.
 *
 * @param tree the tree to traverse
 * @param key the key to consume
 *
 * @return NULL if we couldnt consume the entire stack, the element we end
 *        up at otherwise
 *
 ******************************************************************************/
static struct tree *Traverse(struct tree *tree, snet_util_stack_t *key) {
  int current_int;
  struct numerated_child *current;
  snet_util_stack_iterator_t *key_part;
  key_part = SNetUtilStackBottom(key);
  while(SNetUtilStackIterCurrDefined(key_part)) {
    current_int = SNetUtilStackIterGet(key_part);
    key_part = SNetUtilStackIterNext(key_part);
    SNetUtilListGotoBeginning(tree->children);
    while(SNetUtilListCurrentDefined(tree->children)) {
      current = (struct numerated_child*)SNetUtilListGet(tree->children);
      SNetUtilListNext(tree->children);
      if(current->index > current_int) {
        /* list is sorted => we ran too far => element not in list */
        return NULL;
      } else if(current->index == current_int) {
        tree = current->child;
      } 
    }
  }

  SNetUtilStackIterDestroy(key_part);
  return tree;
}

/**<!--**********************************************************************-->
 *
 * @fn snet_util_tree_t *SNetUtilTreeCreate()
 *
 * @brief creates a new tree
 *
 * @return an emty tree. 
 *
 ******************************************************************************/
snet_util_tree_t *SNetUtilTreeCreate() {
  struct tree *result;
  
  result = malloc(sizeof(struct tree));
  result->content = NULL;
  result->content_defined = false;
  result->children = SNetUtilListCreate();
  
  return result;
}

/**<!--**********************************************************************-->
 *
 * @fn SNetUtilTreeDestroy(snet_util_stack_t *target)
 *
 * @brief destroys the given stack
 *
 * @param target the stack to destroy
 *
 ******************************************************************************/
void SNetUtilTreeDestroy(snet_util_tree_t *target) {
  SNetUtilListGotoBeginning(target->children);
  while(SNetUtilListCurrentDefined(target->children)) {
    SNetUtilTreeDestroy(SNetUtilListGet(target->children));
    SNetUtilListNext(target->children);
  }
  free(target);
}

/**<!--**********************************************************************-->
 *
 * @fn bool SNetUtilTreeContains(snet_util_tree *tree, snet_util_stack *key)
 *
 * @brief checks if the given stack is in the tree.
 *
 *      This checks if the given stack was entered in the tree earlier.
 *      Beware, as this changes the current-element of the deep inspection
 *      of the stack.
 *      Furthermore, tree and key must not be NULL, fatal errors will be 
 *      signaled otherwise.
 *
 * @param tree the tree to search
 * @param key the key to find
 *
 * @return true if this stack is associated with  some value
 *
 ******************************************************************************/
bool SNetUtilTreeContains(snet_util_tree_t *tree, snet_util_stack_t *key) {
  if(tree == NULL) {
    SNetUtilDebugFatal("SnetUtilTreeContains: tree == NULL");
  }
  if(key == NULL) {
    SNetUtilDebugFatal("SNetUtilTreeContains: key == NULL");
  }
  return (Traverse(tree, key)->content_defined);
}

/*<!--***********************************************************************-->
 *
 * @fn void *SNetUtilTreeGet(snet_util_tree_t *tree, snet_util_stack *key)
 *
 * @brief returns the element in the tree.
 *
 *    This returns the element designated by key in the tree. It will
 *    signal some fatal error if the element is not in the tree.
 *    Furthermore, tree and key must not be null. Fatal errors will be signaled
 *    on violation.
 *
 * @param tree the tree to search
 * @param key the key to find
 *
 * @return the element to want
 *
 ******************************************************************************/
void *SNetUtilTreeGet(snet_util_tree_t *tree, snet_util_stack_t *key) {
  struct tree *elem;

  if(tree == NULL) {
    SNetUtilDebugFatal("SNetUtilTreeGet: tree == NULL");
  }
  if(key == NULL) {
    SNetUtilDebugFatal("SNetUtilTreeGet: key == NULL");
  }

  elem = Traverse(tree, key);
  if(elem->content_defined ) {
    return elem->content;
  } else {
    SNetUtilDebugFatal("SnetUtilTreeGet: element not in tree!");
    /* unreachable */
    return NULL;
  }
}

/**<!--**********************************************************************-->
 *
 * @fn void SNetUtilTreeSet(snet_util_tree_t *tree, 
 *                          snet_util_stack *key, void *value)
 *
 * @brief adds the mapping key => value to the tree. If there is some mapping
 *    key=>value already, it is overwritten. 
 *
 *          adds the mapping key=>value to the tree.
 *          Furthermore:
 *              set(t, k, v); contains(t, k) = true
 *              set(t, k, v); get(t, k) = v
 *
 * @param tree the tree to modify
 * @param key the key to add
 * @param the value to set
 *
 */
void SNetUtilTreeSet(snet_util_tree_t *tree, snet_util_stack_t *key,
              void *content)
{
  int current_int;
  struct numerated_child *current;
  struct numerated_child *new_child;
  struct tree *new_tree;
  snet_util_stack_iterator_t *key_part;

  key_part = SNetUtilStackBottom(key);
  while(SNetUtilStackIterCurrDefined(key_part)) {
    current_int = SNetUtilStackIterGet(key_part);
    key_part = SNetUtilStackIterNext(key_part);
    SNetUtilListGotoBeginning(tree->children);
    while(SNetUtilListCurrentDefined(tree->children)) {
      current = (struct numerated_child*)SNetUtilListGet(tree->children);
      SNetUtilListNext(tree->children);
      if(current->index > current_int) {
        /* list is sorted => we ran too far => element not in list */
        new_child = malloc(sizeof(struct numerated_child));
        new_child->index = current_int;
        new_tree = SNetUtilTreeCreate();
        new_child->child = new_tree;
        SNetUtilListAddBefore(tree->children, new_child);
        tree = new_tree;
      } else if(current->index == current_int) {
        tree = current->child;
      }
    }
  }

  SNetUtilStackIterDestroy(key_part);
  tree->content = content;
  tree->content_defined = true;
}

/**<!--**********************************************************************-->
 *
 * @fn void SNetUtilTreeDelete(snet_util_tree *tree, snet_util_stack *key)
 *
 * @brief delets the element from the tree. If its not in there, nothing happens
 *
 * @param tree the tree to manipulate, must not be NULL
 * @param key the key to kill, must not be NULL
 *
 ******************************************************************************/
void SNetUtilTreeDelete(snet_util_tree_t *tree, snet_util_stack_t *key) {
  struct tree *elem;
  if(tree == NULL) {
    SNetUtilDebugFatal("TreeDelete: tree == NULL");
  }
  if(key == NULL) {
    SNetUtilDebugFatal("TreeDelete: key == NULL");
  }
  
  elem = Traverse(tree, key);
  if(elem != NULL) {
    elem->content_defined = false;
  }
}
