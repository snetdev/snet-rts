#ifndef DISTRIBFRONT_H
#define DISTRIBFRONT_H

/* An output manager communicates a new connection
 * to an input manager by means of this 'connect' structure.
 */
typedef struct connect {
  int           source_loc;     /* location of originating host */
  int           source_conn;    /* sender connection identification */
  int           dest_loc;       /* location of receiving host */
  int           table_index;    /* index into node streams table */
  int           cont_loc;       /* location of continuation */
  int           cont_num;       /* index into hash of continuations */
  int           split_level;    /* indexed location placement level */
  int           dyn_loc;        /* current dynamic placement value */
} connect_t;

/* Tags for distribited communication commands. */
typedef enum snet_comm {
  SNET_COMM_connect = 20,       /* setup a new cross-location stream */
  SNET_COMM_stop    = 30,       /* terminate remote input manager */
  SNET_COMM_record  = 40,       /* communicate a record */
} snet_comm_t;

/* The message structure which is constructed by SNetDistribReceiveMessage. */
typedef struct snet_mesg {
  int            type;          /* protocol tag */
  int            source;        /* sender location */
  int            conn;          /* connection identification */
  connect_t     *connect;       /* new connection setup info */
  snet_record_t *rec;           /* transmitted record */
  snet_ref_t    *ref;           /* transmitted reference */
  uintptr_t      data;          /* transmitted data */
  int            val;           /* */
} snet_mesg_t;

void SNetDistribReceiveMessage(snet_mesg_t *mesg);
void SNetDistribTransmitConnect(connect_t *connect);
void SNetDistribTransmitRecord(snet_record_t *rec, connect_t *connect);

#endif
