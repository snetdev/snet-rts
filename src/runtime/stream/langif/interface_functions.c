#include "interface_functions.h"
#include "debug.h"
#include "memfun.h"

static snet_interface_functions_t *snet_interfaces = NULL;

void SNetInterfaceRegister(int id,
                           snet_free_fun_t freefun,
                           snet_copy_fun_t copyfun,
                           snet_alloc_size_fun_t allocsizefun,
                           snet_serialise_fun_t serialisefun,
                           snet_deserialise_fun_t deserialisefun,
                           snet_encode_fun_t encodefun,
                           snet_decode_fun_t decodefun,
                           snet_pack_fun_t packfun,
                           snet_unpack_fun_t unpackfun)
{
  snet_interface_functions_t *new = 
    SNetMemAlloc(sizeof(snet_interface_functions_t));
  new->id = id;
  new->next = NULL;
  new->freefun = freefun;
  new->copyfun = copyfun;
  new->allocsizefun = allocsizefun;
  new->serialisefun = serialisefun;
  new->deserialisefun = deserialisefun;
  new->encodefun = encodefun;
  new->decodefun = decodefun;
  new->packfun = packfun;
  new->unpackfun = unpackfun;

  if (snet_interfaces == NULL) {
      snet_interfaces = new;
  } else {
    snet_interface_functions_t *tmp = snet_interfaces;
    while (tmp->next != NULL) {
      tmp = tmp->next;
    }
    tmp->next = new;
  }
}

snet_interface_functions_t *SNetInterfaceGet(int id)
{
  snet_interface_functions_t *tmp = snet_interfaces;
  while (tmp != NULL && tmp->id != id) {
    tmp = tmp->next;
  }

  if (tmp == NULL) {
    SNetUtilDebugFatal("Interface ID (%d) not found!\n\n", id);
  }

  return tmp;
}

void SNetInterfacesDestroy()
{
  snet_interface_functions_t *tmp = snet_interfaces, *next;
  while (tmp != NULL) {
      next = tmp->next;
      SNetMemFree(tmp);
      tmp = next;
  }
}
