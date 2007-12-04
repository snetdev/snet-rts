#include <stdio.h>
#include <stdlib.h>

#include <sacinterface.h>
#include <snetentities.h>
#include <SAC2SNet.h>
#include <pthread.h>
#include <sudokusolve.h>
#include <memfun.h>
#include <constants.h>
#include <cwrapper.h>

extern void FibreScanIntArray( int** destarray, int **no_use,
                        FILE *stream, int dim, int *shp);


#define SIZE 9
int ITERATIONS;


typedef struct {
 snet_record_t **in_recs;
 snet_buffer_t *inbuf;
} thread_arg_t;

void *Feed( void *ptr) {
  thread_arg_t *inf = (thread_arg_t*)ptr;
  int i;

  for( i=0; i<ITERATIONS; i++) {
    SNetBufPut( inf->inbuf, inf->in_recs[i]);
  }
  printf("Sending Control Record...\n");
  SNetBufPut( inf->inbuf, SNetRecCreate( REC_terminate)); 
  return( NULL);
}

void *Read( void *ptr) 
{
  int i,j,k=0;
  snet_record_t *resrec = NULL;
  snet_buffer_t *outbuf = (snet_buffer_t*)ptr;
  SACarg *output;
  int *resarray1;
  bool terminate = false;

  while( !terminate) {
    resrec = SNetBufGet( outbuf);
    if( SNetRecGetDescriptor( resrec) == REC_terminate) {
      terminate = true;
    }
    else {
      output = SNetRecTakeField( resrec,  F__sudokusolve__board);
      resarray1 = SACARGconvertToIntArray( output);
      printf("\nResult %d/%d\n", k+1, ITERATIONS);
      for( i=0; i<SIZE; i++) {
        for( j=0; j<SIZE; j++) {
          printf(" %d ", resarray1[(i*SIZE)+j]);
        }
        printf("\n");
      }
      free( resarray1);
    }
    SNetRecDestroy( resrec);
    k++;
  }

  return( NULL);
}



int main( int argc, char **argv)
{
  thread_arg_t *inf;
  pthread_t t, t1;

  snet_buffer_t *inbuf, *outbuf;
  snet_record_t *rec1, **in_records;
  snet_variantencoding_t *type1;
  SACarg *input;
  int *dim, savedim;
  int *board, *shape, *useless; 
  int i,j;


  printf( "\n-----------------------------------------------------\n"
          "Sudoku Solver\n"
          "-----------------------------------------------------\n"
          "Usage: %s [num] < puzzle\n"
          "       where [num] specifies number of input records\n" 
          "       (defaults to 1) and 'puzzle' a Sudoku in\n"
          "       fibre format"
          "\n-----------------------------------------------------\n", argv[0]);

  /* CMD line arguments (amount of records) */
  if( argc <= 1) {
    ITERATIONS = 1;
  }
  else {
    ITERATIONS = atoi( argv[1]);
    if( ITERATIONS < 1) {
      ITERATIONS = 1;
    }
  }
  printf("\nNumber of input records: %d\n", ITERATIONS);

  /* Initialise S-Net runtime system */
  SNetGlobalInitialise();
  SAC2SNet_init( 0, SACTYPE_SNet_SNet);


  /* Build SACarg from input board */
  dim = malloc( sizeof( int));
  *dim = 9;
  savedim = *dim;
  
  board = malloc( (SIZE*SIZE) * sizeof( int));
  useless = malloc( (SIZE*SIZE) * sizeof( int));
  shape = malloc( 2*sizeof( int));
  
  shape[0] = SIZE;
  shape[1] = SIZE;

  for( i=0; i<SIZE; i++) {
    for( j=0; j<SIZE; j++) {
      board[(i*SIZE)+j] = 0;
    }
  }
  
  FibreScanIntArray( &board, &useless, stdin, 2, shape);
  
  printf("\nInitial Board:\n\n");
  for( i=0; i<SIZE; i++) {
    for( j=0; j<SIZE; j++) {
      printf(" %d ", board[(i*SIZE)+j]);
    }
    printf("\n");
  }
  printf("\n");

  input = SACARGconvertFromIntPointer( board, 2, SIZE, SIZE);


  /* Build record */
  type1 = SNetTencVariantEncode( 
            SNetTencCreateVector(1 , F__sudokusolve__board),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0));

  rec1 = SNetRecCreate( REC_data, type1);
  SNetRecSetField( rec1, F__sudokusolve__board, input);
  SNetRecSetInterfaceId( rec1, 0);


  /* Build more input records */
  inf = SNetMemAlloc( sizeof( thread_arg_t));
  in_records = SNetMemAlloc( ITERATIONS * sizeof( snet_record_t*));

  printf("\n\nCreating input records...");
  for( i=0; i<ITERATIONS-1; i++) {
    in_records[i] = SNetRecCopy( rec1);
  }
  in_records[ITERATIONS-1] = rec1;


  /* Create input buffer */
  inbuf = SNetBufCreate( BUFFER_SIZE);
  
  /* Start 'feeding' thread */
  inf->inbuf = inbuf;
  inf->in_recs = in_records;
  pthread_create( &t, NULL, Feed, inf);

  /* Invoke network */
  outbuf =  SNet__sudokusolve___sudokusolve( inbuf);
  printf("\n\nStarting to process input...\n\n");

  /* Start 'read' thread */
  pthread_create( &t1, NULL, Read, outbuf);

  /* Wait for Feeder/Reader */
  pthread_join( t, NULL);
  pthread_join( t1, NULL);
  
  printf("\nEnd.\n\n"); 

  return( 0);
}
