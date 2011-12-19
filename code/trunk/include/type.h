#ifndef TYPE_H
#define TYPE_H

typedef enum { field, tag, btag } snet_type_t;

/* These encodings are used by the compiler */
typedef struct {
  int num;
  snet_type_t *type;
} snetc_type_enc_t;

typedef enum {
  nodist = 0,
  mpi,
  scc
} snet_distrib_t;

#endif
