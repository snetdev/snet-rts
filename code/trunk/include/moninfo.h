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


enum moninfo_event {
  EV_BOX_START,
  EV_BOX_FINISH,
  EV_SYNC_FIRST,
  EV_SYNC_FIRE,
};

enum moninfo_descr {
  MON_RECORD,
};


/* data structure of system-wide unique id vector */
typedef struct {
  unsigned int ids [3];  /* fields: 0..local_id, 1..thread_id, 2..node_id (distributed snet) */
} snet_moninfo_id_t;


/* data structure of monitoring information for records */
typedef struct {
  snet_moninfo_id_t id;
  snet_moninfo_id_t *parents;
  unsigned int time;  /* time stamp of monitoring e */
  char *add_data; /* container for additional arbitrary data */
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
 * Create unique system-wide id
 ****************************************************************************/
snet_moninfo_id_t SNetMonInfoCreateID(void);


#endif /* MONINFO_H_ */
