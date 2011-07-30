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


#define LIST_NAME MonInfoId /* SNetMonInfoIdListFUNC */
#define LIST_TYPE_NAME monid
#define LIST_VAL snet_moninfo_id_t
#define LIST_CMP SNetMonInfoCmpID
#include "list-template.c"
#undef LIST_CMP
#undef LIST_VAL
#undef LIST_TYPE_NAME
#undef LIST_NAME


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
      REC_MONINFO( mon, id) = va_arg( args, snet_moninfo_id_t);
      REC_MONINFO( mon, parent_ids) = va_arg( args, snet_monid_list_t *);
      REC_MONINFO( mon, add_moninfo_rec_data) = va_arg( args, snet_add_moninfo_rec_data_t);
      break;
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
    if (REC_MONINFO( mon, parent_ids) != NULL) SNetMonInfoIdListDestroy( REC_MONINFO( mon, parent_ids));
    if (REC_MONINFO( mon, add_moninfo_rec_data) != NULL) SNetMemFree( REC_MONINFO( mon, add_moninfo_rec_data));
    SNetMemFree( MONINFOPTR( mon));
    break;
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
 * Compares two monitoring information identifiers
 ****************************************************************************/
bool SNetMonInfoCmpID (snet_moninfo_id_t monid1, snet_moninfo_id_t monid2)
{
   bool res = true;
   int i = sizeof(snet_moninfo_id_t) / sizeof (unsigned long);
   for (i--;i>=0;i--)
     if (monid1.ids[i] != monid2.ids[i])
       res = false;
   return res;
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


/*****************************************************************************
 *
 ****************************************************************************/
void SNetMonInfoEvent(snet_moninfo_event_t event, snet_moninfo_descr_t descr,... )
{
  snet_moninfo_t *mon;
  va_list args;

  va_start( args, descr);
  switch (descr) {
  case MON_RECORD: /* monitoring of a record */
    {
      snet_record_t *rec;
      rec = va_arg( args, snet_record_t *);

      switch ( REC_DESCR( rec)) {
      case REC_data: /* currently only data records can be monitored this way */

        switch (event) {
        case EV_BOX_START:
        case EV_SYNC_FIRST:
          /* signature: SNetMonInfoEvent(snet_moninfo_event_t event, snet_moninfo_descr_t, snet_record_t* ) */
          /* action: create moninfo data */
          {
            snet_monid_list_t *parent_id_list = SNetMonInfoIdListCopy( DATA_REC( rec, parent_ids));
            snet_add_moninfo_rec_data_t add_data = SNetMonInfoRecCopyAddData (DATA_REC( rec, add_moninfo_rec_data));

            mon = SNetMonInfoCreate ( event, descr, DATA_REC( rec, id), parent_id_list, add_data);
          }
          break;
        case EV_BOX_FINISH:
          /* signature: SNetMonInfoEvent(snet_moninfo_event_t event, snet_moninfo_descr_t descr, snet_record_t*, snet_record_t* ) */
          /* action: set single parent_id in record and create moninfo data */
          {
            snet_record_t *parent_rec = va_arg( args, snet_record_t *);
            snet_monid_list_t *parent_id_list = SNetMonInfoIdListCreate(1, parent_rec);
            snet_add_moninfo_rec_data_t add_data = SNetMonInfoRecCopyAddData (DATA_REC( rec, add_moninfo_rec_data));
            snet_moninfo_id_t newid = SNetMonInfoCreateID();

            /* set the parent id of the record */
            DATA_REC( rec, parent_ids) = SNetMonInfoIdListCopy(parent_id_list);
            DATA_REC( rec, id) = newid;
            mon = SNetMonInfoCreate ( event, descr, newid, parent_id_list, add_data);
          }
          break;
        case EV_SYNC_FIRE:
          /* signature: SNetMonInfoEvent(snet_moninfo_event_t event, snet_moninfo_descr_t descr, snet_record_t*, snet_record_t* ) */
          /* action: set parent_id_list in record and create moninfo data */

          //FIXME: add code for EV_SYNC_FIRE
          break;
        default:
          SNetUtilDebugFatal("Unknown monitoring information event. [%d]", event);
        } /* switch(event) */
        break;
      default:
        SNetUtilDebugFatal("Non-supported monitoring information record. [%d]", event);
      } /* REC_DESCR( rec) */
    }
    break;
  default:
    SNetUtilDebugFatal("Unknown monitoring information description. [%d]", descr);
  } /* switch (descr) */
  va_end( args);
}


