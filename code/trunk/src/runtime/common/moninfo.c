/*
 * moninfo.c
 *
 *  Created on: 29. July  2011
 *      Author: Raimund Kirner
 *
 * This file includes data types and functions to maintain monitoring information.
 * The main purpose at creation (July 2011) was to have monitoring information for
 * each S-Net record to log the overall execution time in a (sub)network from reading
 * a record until the output of all of its descendant records at the exit of this
 * (sub)network.
 *
 */

#include "moninfo.h"
#include "memfun.h"
#include "threading.h"
#include "debug.h"
#include "string.h"


static unsigned int moninfo_local_id = 0; /* sequence number to create ids */


/*****************************************************************************
 * Create monitoring information (entries depend on monitoring item)
 ****************************************************************************/
snet_moninfo_t *SNetMonInfoCreate ( snet_moninfo_event_t event, snet_moninfo_descr_t descr,... )
{
  snet_moninfo_t *mon;
  va_list args;

  mon = SNetMemAlloc( sizeof( snet_moninfo_t));
  MONINFO_DESCR( mon) = descr;
  MONINFO_EVENT( mon) = event;

  va_start( args, descr);
  switch (descr) {
    case MON_RECORD:
      MONINFOPTR( mon) = SNetMemAlloc( sizeof( snet_moninfo_record_t));
      REC_MONINFO( mon, id) = SNetMonInfoCreateID();
      REC_MONINFO( mon, parent_ids) = va_arg( args, snet_moninfo_id_t *);
      REC_MONINFO( mon, add_moninfo_rec_data) = va_arg( args, snet_add_moninfo_rec_data_t);
  default:
    SNetUtilDebugFatal("Unknown monitoring information description. [%d]", descr);
    break;
  }
  va_end( args);
  return mon;
}


/*****************************************************************************
 * Delete monitoring information (entries depend on monitoring item)
 ****************************************************************************/
void SNetMonInfoDestroy( snet_moninfo_t *mon)
{
  switch (MONINFO_DESCR( mon)) {
  case MON_RECORD:
    if (REC_MONINFO( mon, parent_ids) != NULL) SNetMemFree( REC_MONINFO( mon, parent_ids));
    if (REC_MONINFO( mon, add_moninfo_rec_data) != NULL) SNetMemFree( REC_MONINFO( mon, add_moninfo_rec_data));
    SNetMemFree( MONINFOPTR( mon));
  default:
    SNetUtilDebugFatal("Unknown monitoring information description. [%d]", MONINFO_DESCR( mon));
    break;
  }
}


/*****************************************************************************
 * Create unique system-wide id
 ****************************************************************************/
snet_moninfo_id_t SNetMonInfoCreateID(void)
{
  snet_moninfo_id_t id;
  id.ids[0] = moninfo_local_id++;
  id.ids[1] = SNetThreadingGetId();
  id.ids[2] = 0;  /* FIXME: add code to get node id (distributed snet) */
  return id;
}

/*****************************************************************************
 * Create a copy of the additional data of record monitoring information
 ****************************************************************************/
snet_add_moninfo_rec_data_t SNetMonInfoRecCopyAddData(snet_add_moninfo_rec_data_t add_data)
{
  snet_add_moninfo_rec_data_t new_add_data = (snet_add_moninfo_rec_data_t) strdup ( add_data);
  if (new_add_data == NULL)
    SNetUtilDebugFatal("Copy of additional monitoring information for records failed. [%s]", add_data);
  return new_add_data;
}


