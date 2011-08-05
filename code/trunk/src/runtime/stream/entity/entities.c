#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "memfun.h"
#include "locvec.h"

#include "entities.h"


#define ENT_DESCR(ent)  ((ent)->descr)

#define ENT(ent,name)  ((ent)->name)

struct snet_entity_t {
  snet_entity_descr_t descr;
  int node;
  snet_locvec_t *locvec;
  const char *name;
  snet_entityfunc_t func;
  void *arg;
  const char *str;
};


static char *StringCopy(const char *str)
{
  char *newstr = NULL;

  /* copy non-empty string */
  if (str != NULL) {
    int len = strlen(str);
    if (len > 0) {
      newstr = strncpy(
          SNetMemAlloc((len+1)*sizeof(char)),
          str,
          len+1
          );
    }
  }

  return newstr;
}


/**
 * Create an entity
 *
 * @param descr    the entity type
 * @param node     the node passed to the creation function
 * @param locvec   the location vector
 * @param name     optional name of entity
 * @param func     the entity thread function
 * @param arg      the argument for the entity thread
 *
 * @return the new entity
 */
snet_entity_t *SNetEntityCreate(snet_entity_descr_t descr, ...)
{
  va_list args;
  snet_entity_t *ent = SNetMemAlloc(sizeof(snet_entity_t));

  ENT_DESCR(ent) = descr;
  va_start(args, descr);
  ENT(ent, node) = va_arg(args, int);

  {
    snet_locvec_t *locvec = va_arg(args, snet_locvec_t*);
    /* if locvec is NULL then entity_other */
    assert(locvec != NULL || descr == ENTITY_other);

    ENT(ent, locvec) = (locvec!=NULL)? SNetLocvecCopy(locvec) : NULL;
  }
  {
    const char *name = va_arg(args, const char*);
    ENT(ent, name) = StringCopy(name);
  }
  ENT(ent, func) = va_arg(args, snet_entityfunc_t);
  ENT(ent, arg)  = va_arg(args, void*);

  ENT(ent, str) = NULL;

  va_end(args);

  return ent;
}

void SNetEntityDestroy(snet_entity_t *ent)
{
  assert(ent != NULL);

  if (ENT(ent, locvec) != NULL) {
    SNetLocvecDestroy( ENT(ent, locvec));
  }
  if (ENT(ent, name) != NULL) {
    SNetMemFree( (void*) ENT(ent, name));
  }
  if (ENT(ent, str) != NULL) {
    SNetMemFree( (void*) ENT(ent, str));
  }
  SNetMemFree(ent);
}



snet_entity_t *SNetEntityCopy(snet_entity_t *ent)
{
  assert(ent != NULL);

  snet_entity_t *newent = SNetMemAlloc(sizeof(snet_entity_t));
  ENT_DESCR(newent)   = ENT_DESCR(ent);
  ENT(newent, node)   = ENT(ent, node);
  ENT(newent, locvec) = (ENT(ent, locvec)!=NULL) ? SNetLocvecCopy(ENT(ent, locvec)) : NULL;
  ENT(newent, name)   = StringCopy(ENT(ent, name));
  ENT(newent, func)   = ENT(ent, func);
  ENT(newent, arg)    = ENT(ent, arg);
  ENT(newent, str)    = StringCopy(ENT(ent, str));

  return newent;
}


/**
 * Call the entity funciton.
 *
 * This function must be called from within the threading layer,
 * on a separate thread of execution.
 *
 */
void SNetEntityCall(snet_entity_t *ent)
{
  ENT(ent,func)( ent, ENT(ent,arg) );
}





snet_entity_descr_t SNetEntityDescr(snet_entity_t *ent)
{
  return ENT_DESCR(ent);
}


snet_locvec_t *SNetEntityGetLocvec(snet_entity_t *ent)
{
  return ENT(ent, locvec);
}

int SNetEntityNode(snet_entity_t *ent)
{
  return ENT(ent, node);
}

const char *SNetEntityName(snet_entity_t *ent)
{
  const char *name = ENT(ent, name);
  return (name!=NULL) ? name : "";
}


/**
 * Return the string representation of the entity.
 *
 * It is of the format "<locvec> <name" or only "<name>",
 * depending on whether locvec is NULL or not.
 * The string is cached internally such that it is not recomputed
 * every time SNetEntityStr is called.
 *
 * @note The caller must not free the returned pointer.
 *       If the string needs to be used past the lifetime of the entity,
 *       the caller must make a copy of the string.
 */
const char *SNetEntityStr(snet_entity_t *ent)
{
  const char *name = ENT(ent, name);
  if (ENT(ent, locvec) != NULL) {
    /* if not already cached, generate a new one*/
    if (ENT(ent, str) == NULL) {
      int size, namelen, len;
      char *buf;

      if (name==NULL) { name = ""; }
      namelen = strlen(name);
      /* initial size of the buffer */
      size = 32;

      while(1) {
        int space = size-(namelen+2);
        buf = SNetMemAlloc(size * sizeof(char));
        len = SNetLocvecPrint(buf, space, ENT(ent,locvec));
        if (len>=space) {
          size *= 2;
          SNetMemFree(buf);
        } else {
          buf[len] = ' ';
          (void) strncpy(buf+len+1, name, namelen+1);
          break;
        }
      }
      len = strlen(buf)+1;
      ENT(ent, str) =  strncpy( SNetMemAlloc(len), buf, len);
      SNetMemFree(buf);
    }
    return ENT(ent, str);
  } else {
    /* return name */
    return (name!=NULL) ? name : "";
  }
}

