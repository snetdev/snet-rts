

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memfun.h>
#include <graphics.h>

#include "graphicFactorial.h"


#define NUM_NAMES 14
char **names;





#define PRINT_RECORD( NAME)\
    printf("\n - %2d fields: ", SNetRecGetNumFields( NAME));\
    for( k=0; k<SNetRecGetNumFields( NAME); k++) {\
      printf(" %s=%d ", names[ SNetRecGetFieldNames( NAME)[k] ],\
                        *((int*)SNetRecGetField( NAME, SNetRecGetFieldNames( NAME)[k])));\
    }\
    printf("\n - %2d tags  : ", SNetRecGetNumTags( NAME));\
    for( k=0; k<SNetRecGetNumTags( NAME); k++) {\
      printf(" %s=%d ", names[ SNetRecGetTagNames( NAME)[k] ],\
                               SNetRecGetTag( NAME, SNetRecGetTagNames( NAME)[k]));\
    }\
    printf("\n - %2d btags : ", SNetRecGetNumBTags( NAME));\
    for( k=0; k<SNetRecGetNumBTags( NAME); k++) {\
      printf(" %s=%d ", names [ SNetRecGetBTagNames( NAME)[k] ],\
                        (int)SNetRecGetBTag( NAME, SNetRecGetBTagNames( NAME)[k]));\
    }


void initialize() {

  names = SNetMemAlloc( NUM_NAMES * sizeof( char*));
  
  names[0] = "F_x";
  names[1] = "F_r";
  names[2] = "F_xx";
  names[3] = "F_rr";
  names[4] = "F_one";
  names[5] = "F_p";
  names[6] = "T_T";
  names[7] = "T_F";
  names[8] = "T_out";
  names[9] = "T_ofl";
  names[10] = "T_stop";
  names[11] = "F_z";
  names[12] = "T_s";
  names[13] = "BT_tst";


  SNetInitGraphicalSystem();
}




int main() {
  snet_typeencoding_t *enc1, *enc2, *enc3;
  snet_record_t *rec1, *rec2, *rec3, *resrec;
  snet_buffer_t *start_buf, *res_buf;

  int i,j,k;
  char *cbuf;

  int *F_x_1, *F_x_2, *F_x_3, *F_y_1, *F_y_2, *F_y_3, *F_z_1;

  cbuf = SNetMemAlloc( 2 * sizeof( char));
  

  initialize();

  enc1 = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 3, F_x, F_r, F_z), 
            SNetTencCreateVector( 0), 
            SNetTencCreateVector( 0)));

  enc2 = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 2, F_x, F_r), 
            SNetTencCreateVector( 0), 
            SNetTencCreateVector( 0)));

  enc3 = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 2, F_x, F_r), 
            SNetTencCreateVector( 0), 
            SNetTencCreateVector( 0)));

  rec1 = SNetRecCreate( REC_data, SNetTencGetVariant( enc1, 1));
  rec2 = SNetRecCreate( REC_data, SNetTencGetVariant( enc2, 1));
  rec3 = SNetRecCreate( REC_data, SNetTencGetVariant( enc3, 1));



  F_x_1 = SNetMemAlloc( sizeof( int));
  F_x_2 = SNetMemAlloc( sizeof( int));
  F_x_3 = SNetMemAlloc( sizeof( int));
  F_z_1 = SNetMemAlloc( sizeof( int));
  F_y_1 = SNetMemAlloc( sizeof( int));
  F_y_2 = SNetMemAlloc( sizeof( int));
  F_y_3 = SNetMemAlloc( sizeof( int));

  *F_x_1 = 4; 
  *F_y_1 = 1;

  *F_x_2 = 2;
  *F_y_2 = 1;
  
  *F_x_3 = 11;
  *F_y_3 = 1;
 
  *F_z_1 = 42;

  SNetRecSetField( rec1, F_x, (void*)F_x_1);
  SNetRecSetField( rec1, F_z, (void*)F_z_1);
  SNetRecSetField( rec2, F_x, (void*)F_x_2);
  SNetRecSetField( rec3, F_x, (void*)F_x_3);

  SNetRecSetField( rec1, F_r, (void*)F_y_1);
  SNetRecSetField( rec2, F_r, (void*)F_y_2);
  SNetRecSetField( rec3, F_r, (void*)F_y_3);


  SNetRecAddTag( rec1, T_s);
  SNetRecSetTag( rec1, T_s, 43);

  start_buf = SNetBufCreate( 10);

/*
  SNetBufPut( start_buf, rec1);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec1);
  printf("\n");

  SNetBufPut( start_buf, rec2);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec2);
  printf("\n"); 

  SNetBufPut( start_buf, rec3);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec3);
  printf("\n");
*/
  printf("\n\n\n");

  res_buf = SER_net_graph(  SNetGraphicalFeeder( start_buf, (char**)names, NUM_NAMES));



  printf("\n  > Wait,  then press a key to terminate this"
         " application. <\n");
  getc( stdin);

  i = BUFFER_SIZE-SNetGetSpaceLeft( res_buf);
  printf("\n\nResulting Buffer contains %d records.\n", i);
    for( j=0; j<i; j++) {
    printf("\nRecord %d:", j);
    resrec = SNetBufGet( res_buf);
    if( SNetRecGetDescriptor( resrec) == REC_data) {
      PRINT_RECORD( resrec);
      printf("\n");
    }
    else {
      printf("\n - Control Record, Type: %d\n", SNetRecGetDescriptor( resrec));
    } 
  }
  
  
  printf("\nEnd.\n"); 



  return( 0);
}


