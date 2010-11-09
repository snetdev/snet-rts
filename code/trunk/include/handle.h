/* 
 * $Id$
 */

#ifndef HANDLE_H
#define HANDLE_H

#include "typeencode.h"
#include "expression.h"

struct handle;
typedef struct handle snet_handle_t;

#include "record.h"

extern snet_record_t *SNetHndGetRecord( snet_handle_t *hndl);
extern snet_box_sign_t *SNetHndGetBoxSign( snet_handle_t *hnd);

#endif
