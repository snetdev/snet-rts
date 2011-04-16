/* 
 * $Id$
 */

#ifndef HANDLE_H
#define HANDLE_H

struct handle;
typedef struct handle snet_handle_t;

#include "record.h"

extern snet_record_t *SNetHndGetRecord( snet_handle_t *hndl);
extern snet_variant_list_t *SNetHndGetVariants( snet_handle_t *hnd);
#endif
