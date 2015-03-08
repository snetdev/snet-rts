#ifndef MPI_COMM_WORLD
 #include <mpi.h>
#endif
#include <string.h>

#include "memfun.h"

#define MPI_VOID_POINTER_TEST(b,t) (sizeof(void*) == sizeof(b)) ? t : 
#define MPI_VOID_POINTER (MPI_VOID_POINTER_TEST(int,MPI_UNSIGNED) \
                          MPI_VOID_POINTER_TEST(long,MPI_UNSIGNED_LONG) \
                          MPI_VOID_POINTER_TEST(long long,MPI_UNSIGNED_LONG_LONG) \
                          MPI_VOID_POINTER_TEST(short,MPI_UNSIGNED_SHORT) \
                          MPI_VOID_POINTER_TEST(char,MPI_BYTE) \
                          MPI_DATATYPE_NULL)

typedef struct {
  int           offset;
  size_t        size;
  char         *data;
} mpi_buf_t;

inline static void MPIPack(mpi_buf_t *buf, void *src,
                           MPI_Datatype type, int count)
{
  int size;

  MPI_Pack_size(count, type, MPI_COMM_WORLD, &size);
  if (buf->offset + size > buf->size) {
    buf->data = SNetMemResize(buf->data, buf->offset + size);
    buf->size = buf->offset + size;
  }
  MPI_Pack(src, count, type, buf->data, buf->size, &buf->offset, MPI_COMM_WORLD);
}

inline static void MPIUnpack(mpi_buf_t *buf, void *dst,
                             MPI_Datatype type, int count)
{
  MPI_Unpack(buf->data, buf->size, &buf->offset, dst, count, type, MPI_COMM_WORLD);
}

void SNetMPISend(void *src, int size, int dest, int type);
void SNetPackInt(void *buf, int count, int *src);
void SNetUnpackInt(void *buf, int count, int *dst);
void SNetPackByte(void *buf, int count, char *src);
void SNetUnpackByte(void *buf, int count, char *dst);
void SNetPackLong(void *buf, int count, long *src);
void SNetUnpackLong(void *buf, int count, long *dst);
void SNetPackVoid(void *buf, int count, void **src);
void SNetUnpackVoid(void *buf, int count, void **dst);
void SNetPackRef(void *buf, int count, snet_ref_t **src);
void SNetUnpackRef(void *buf, int count, snet_ref_t **dst);
void startMonMPI();
void stopMonMPI();

