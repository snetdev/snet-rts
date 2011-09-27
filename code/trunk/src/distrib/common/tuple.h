#ifndef TUPLE_H
#define TUPLE_H

typedef struct {
  snet_dest_t *dest;
  snet_stream_t *stream;
} snet_tuple_t;

inline static bool SNetTupleCompare(snet_tuple_t t1, snet_tuple_t t2)
{
  (void) t1; /* NOT USED */
  (void) t2; /* NOT USED */
  return false;
}
#endif
