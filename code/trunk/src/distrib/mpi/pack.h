#include <mpi.h>

typedef struct {
  int offset;
  size_t size;
  char *data;
} mpi_buf_t;

void MPIPack(mpi_buf_t *buf, void *src, MPI_Datatype type, int count);
void MPIUnpack(mpi_buf_t *buf, void *dst, MPI_Datatype type, int count);
