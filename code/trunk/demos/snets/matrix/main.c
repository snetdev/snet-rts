#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memfun.h>

#include <C4SNet.h>
#include "bool.h"
#include "distribution.h"
#include "fun.h"

#include "matrix.h"

#define PRINT_RECORD( NAME) __PRINT_RECORD( NAME, false)

#define __PRINT_RECORD( NAME, FREE)\
    printf("\n - %2d fields: ", SNetRecGetNumFields( NAME));\
    for( k=0; k<SNetRecGetNumFields( NAME); k++) {\
      printf(" %s=%d ", snet_matrix_labels[ SNetRecGetFieldNames( NAME)[k] ],\
                        *((int*)C4SNetDataGetData( SNetRecGetField( NAME, SNetRecGetFieldNames( NAME)[k]))));\
    }\
    printf("\n - %2d tags  : ", SNetRecGetNumTags( NAME));\
    for( k=0; k<SNetRecGetNumTags( NAME); k++) {\
      printf(" %s=%d ", snet_matrix_labels[ SNetRecGetTagNames( NAME)[k] ],\
                               SNetRecGetTag( NAME, SNetRecGetTagNames( NAME)[k]));\
    }\
    printf("\n - %2d btags : ", SNetRecGetNumBTags( NAME));\
    for( k=0; k<SNetRecGetNumBTags( NAME); k++) {\
      printf(" %s=%d ", snet_matrix_labels[ SNetRecGetBTagNames( NAME)[k] ],\
                        (int)SNetRecGetBTag( NAME, SNetRecGetBTagNames( NAME)[k]));\
    }
    
char **names;

void initialise() {

  SNetGlobalInitialise();
  C4SNetInit( 0); 
}

#define A_W 1024
#define A_H 256
#define B_W 512
#define B_H A_W

void *InputThread(void *ptr) 
{
  snet_record_t *rec;
  snet_tl_stream_t *start_stream;

  int *A, *B;
  int i, j;
  int k;

  c4snet_data_t *field_A, *field_B;

  int nodes;

  start_stream = DistributionWaitForInput(); 

  if(start_stream != NULL) {

    rec = SNetRecCreate( REC_data,
             SNetTencVariantEncode( 
	      SNetTencCreateVector( 2, F__matrix__A, F__matrix__B),
	      SNetTencCreateVector( 5, T__matrix__A_width, T__matrix__A_height, T__matrix__B_width, T__matrix__B_height, T__matrix__nodes), 
              SNetTencCreateVector( 0)));
    


    A = (int *)SNetMemAlloc(sizeof(int) * A_W * A_H);
    B = (int *)SNetMemAlloc(sizeof(int) * B_W * B_H);

    for(i = 0; i < A_H; i++) {
      for(j = 0; j < A_W; j++) {
	(A + A_W * i)[j] = j + 1;
      }
    }

    for(i = 0; i < B_H; i++) {
      for(j = 0; j < B_W; j++) {
	(B + B_W * i)[j] = i + 1;
      }
    }
    
    MPI_Comm_size(MPI_COMM_WORLD, &nodes);

    field_A = C4SNetDataCreateArray(CTYPE_int, A_W*A_H, A);
    field_B = C4SNetDataCreateArray(CTYPE_int, B_W*B_H, B);
   
    SNetRecSetInterfaceId(rec, 0);
    SNetRecSetField(rec, F__matrix__A, (void*)field_A);
    SNetRecSetField(rec, F__matrix__B, (void*)field_B);
    SNetRecSetTag(rec, T__matrix__A_width, A_W);
    SNetRecSetTag(rec, T__matrix__A_height, A_H);
    SNetRecSetTag(rec, T__matrix__B_width, B_W);
    SNetRecSetTag(rec, T__matrix__B_height, B_H);
    SNetRecSetTag(rec, T__matrix__nodes, nodes);

    printf("\nPut record to Buffer:");
    PRINT_RECORD(rec);
    printf("\n");
    SNetTlWrite(start_stream, rec);
    
    printf("\n\n\n");
  
    printf("put terminate\n");
    SNetTlWrite(start_stream, SNetRecCreate( REC_terminate));

    SNetTlMarkObsolete(start_stream); 
  }

  return NULL;
}

void *OutputThread(void *ptr) 
{
  snet_record_t *resrec;
  snet_tl_stream_t *res_stream;
  bool terminate;

  int j,k,i;

  int *data;
  int x,y;

  FILE *fp;

  if((fp = fopen("result.txt", "w")) == 0) {
    exit(1);
  }

  res_stream = DistributionWaitForOutput();

  if(res_stream != NULL) {
    for( j=0, terminate = false; !terminate; j++) {
      resrec = NULL;
      resrec = SNetTlRead( res_stream);
      if( SNetRecGetDescriptor( resrec) == REC_data) {
	
	  data = C4SNetDataGetData(SNetRecGetField(resrec, F__matrix__C));
	  x = SNetRecGetTag(resrec, T__matrix__C_width);
	  y = SNetRecGetTag(resrec, T__matrix__C_height);

	  for(i = 0; i < y; i++) {
	    for(k = 0; k < x; k++) {
	      fprintf(fp, "%d ", (data + i * x)[k]);
	    }
	    fprintf(fp, "\n");
	  }
	  printf("\n\n");
	
	  PRINT_RECORD( resrec);  
	  printf("\n");
      }
      else {
	printf("\n - Control Record, Type: %d\n", SNetRecGetDescriptor( resrec));
	if(SNetRecGetDescriptor( resrec) == REC_terminate) {
	  terminate = true;
	}
      }
      SNetRecDestroy( resrec);
    }
  }

  fclose(fp);
 
  return NULL;
}

int main(int argc, char *argv[]) 
{
  pthread_t ithread;
  int node;

  initialise();

  SNetDistFunRegisterLibrary("matrix", 
			     SNetFun2ID_matrix,
			     SNetID2Fun_matrix);


  node = DistributionInit(argc, argv);

  if(node == 0) {
    DistributionStart(SNet__matrix___matrix);
  }
   
  pthread_create(&ithread, NULL, InputThread, NULL);
  pthread_detach(ithread);

  OutputThread(NULL);

  if(node == 0) {
    DistributionStop();
  }

  DistributionDestroy();

  SNetGlobalDestroy();

  return( 0);
}
// -f "Elapsed: %e\n User: %U\n Kernel: %S\n Percentage: %P\n"

