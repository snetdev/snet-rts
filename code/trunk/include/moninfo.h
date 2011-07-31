/*
 * moninfo.h
 *
 *  Created on: Jul 29, 2011
 *      Author: Raimund Kirner
 * This file includes data types and functions to maintain monitoring information.
 * The main purpose at creation (July 2011) was to have monitoring information for
 * each S-Net record to log the overall execution time in a (sub)network from reading
 * a record until the output of all of its descendant records at the exit of this
 * (sub)network.
 *
 */

#ifndef MONINFO_H_
#define MONINFO_H_

typedef enum moninfo_event snet_moninfo_event_t;
typedef enum moninfo_descr snet_moninfo_descr_t;
typedef union moninfo_types snet_moninfo_types_t;

/* macros for monitoring information */
#define MONINFO_DESCR( name) name->mon_descr
#define MONINFO_EVENT( name) name->mon_event
#define MONINFOPTR( name) name->mon_data

#define REC_MONINFO( name, component) MONINFOPTR( name) ->moninfo_rec.component

/* data structure of system-wide unique id vector */
typedef struct {
  unsigned long ids [3];  /* fields: 0..local_id, 1..thread_id, 2..node_id (distributed snet) */
} snet_moninfo_id_t;

#include "bool.h"
#define LIST_NAME_H MonInfoId /* SNetMonInfoIdListFUNC */
#define LIST_TYPE_NAME_H monid
#define LIST_VAL_H snet_moninfo_id_t
#include "list-template.h"
#undef LIST_VAL_H
#undef LIST_TYPE_NAME_H
#undef LIST_NAME_H

enum moninfo_event {
  EV_INPUT_ARRIVE,
  EV_BOX_START,
  EV_BOX_WRITE,
  EV_BOX_FINISH,
  EV_SYNC_FIRST,
  EV_SYNC_FIRE,
};

enum moninfo_descr {
  MON_RECORD,
};


/* data structure of additional monitoring information of records (e.g., shape, etc.) */
typedef char* snet_add_moninfo_rec_data_t;

/* data structure of monitoring information for records */
typedef struct {
  snet_moninfo_id_t id;
  //snet_moninfo_id_t *parent_ids;
  snet_monid_list_t *parent_ids;
  unsigned int time;  /* time stamp of monitoring e */
  snet_add_moninfo_rec_data_t add_moninfo_rec_data; /* container for additional arbitrary data */
} snet_moninfo_record_t;


/* enum of different monitoring info structures */
union moninfo_types {
  snet_moninfo_record_t moninfo_rec;
};


/* data structure of monitoring information */
typedef struct snet_moninfo {
  snet_moninfo_event_t mon_event;
  snet_moninfo_descr_t mon_descr;
  snet_moninfo_types_t *mon_data;
} snet_moninfo_t;


/*****************************************************************************
 * Create monitoring information (entries depend on monitoring item)
 ****************************************************************************/
snet_moninfo_t *SNetMonInfoCreate ( snet_moninfo_event_t event, snet_moninfo_descr_t descr,... );


/*****************************************************************************
 * Delete monitoring information (entries depend on monitoring item)
 ****************************************************************************/
void SNetMonInfoDestroy( snet_moninfo_t *mon);



/*****************************************************************************
 * Create unique system-wide id
 ****************************************************************************/
snet_moninfo_id_t SNetMonInfoCreateID(void);


/*****************************************************************************
 * Compares two monitoring information identifiers
 ****************************************************************************/
bool SNetMonInfoCmpID (snet_moninfo_id_t monid1, snet_moninfo_id_t monid2);


/*****************************************************************************
 * Create a copy of the additional data of record monitoring information
 ****************************************************************************/
snet_add_moninfo_rec_data_t SNetMonInfoRecCopyAddData(snet_add_moninfo_rec_data_t add_data);


/*****************************************************************************
 * Trigger the output of some monitoring information
 ****************************************************************************/
void SNetMonInfoEvent(snet_moninfo_event_t event, snet_moninfo_descr_t descr,... );



#endif /* MONINFO_H_ */
