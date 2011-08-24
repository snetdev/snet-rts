/* 
 * $Id$
 */

#ifndef _HANDLE_H_
#define _HANDLE_H_

struct handle;
typedef struct handle snet_handle_t;

#include "list.h"
#include "record.h"

extern snet_record_t *SNetHndGetRecord( snet_handle_t *hndl);
extern snet_int_list_list_t *SNetHndGetVariants( snet_handle_t *hnd);
extern snet_variant_list_t *SNetHndGetVariantList( snet_handle_t *hnd);

#endif /* _HANDLE_H_ */
