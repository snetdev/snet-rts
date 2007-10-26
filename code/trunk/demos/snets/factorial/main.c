#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memfun.h>

#include <C2SNet.h>
#include <myfuns.h>
#include "factorial.h"


#define PRINT_RECORD_WITH_FREE( NAME) __PRINT_RECORD( NAME, true)
#define PRINT_RECORD( NAME) __PRINT_RECORD( NAME, false)


#define __PRINT_RECORD( NAME, FREE)\
     printf("\n - %2d fields: ", SNetRecGetNumFields( NAME));\
    for( k=0; k<SNetRecGetNumFields( NAME); k++) {\
      printf(" %s=%d ", names[ SNetRecGetFieldNames( NAME)[k] ],\
                        *((int*)C2SNet_cdataGetData( SNetRecGetField( NAME, SNetRecGetFieldNames( NAME)[k]))));\
      if( FREE) {\
        SNetMemFree( SNetRecGetField( NAME, SNetRecGetFieldNames( NAME)[k]));\
      }\
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
    
#define NUM_NAMES 14

char **names;

void initialise() {

  SNetGlobalInitialise();
  C2SNet_init( 0);

  names = SNetMemAlloc( NUM_NAMES * sizeof( char*));
  
  names[0] = "F_none";
  names[1] = "F_x";
  names[2] = "F_p";
  names[3] = "F_xx";
  names[4] = "F_r";
  names[5] = "F_rr";
  names[6] = "T_T";
  names[7] = "T_F";
  names[8] = "T_stop";
  names[9] = "T_ofl";
  names[10] = "T_stop";
  names[11] = "F_z";
  names[12] = "T_s";
  names[13] = "BT_tst";
 
}
#define F_x 1
#define F_r 4
#define F_z 99

int main() {

  snet_typeencoding_t *enc1, *enc2, *enc3, *enc4, *enc5, *enc6;
  snet_record_t *rec1, *rec2, *rec3, *rec4, *rec5, *rec6, *resrec;
  snet_buffer_t *start_buf, *res_buf;

  int i,j,k;

  int *F_x_1, *F_x_2, *F_x_3, *F_x_4, *F_x_5, *F_x_6, *F_r_1, *F_r_2, *F_r_3, *F_r_4, *F_r_5, *F_r_6, *F_z_1;

  C_Data *field1, *field2, *field3, *field4;



  initialise();



  enc1 = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 2, F_x, F_r), 
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

  enc4 = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 2, F_x, F_r), 
            SNetTencCreateVector( 0), 
            SNetTencCreateVector( 0)));

  enc5 = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 2, F_x, F_r), 
            SNetTencCreateVector( 0), 
            SNetTencCreateVector( 0)));

  enc6 = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 2, F_x, F_r), 
            SNetTencCreateVector( 0), 
            SNetTencCreateVector( 0)));

  rec1 = SNetRecCreate( REC_data, SNetTencGetVariant( enc1, 1));
  rec2 = SNetRecCreate( REC_data, SNetTencGetVariant( enc2, 1));
  rec3 = SNetRecCreate( REC_data, SNetTencGetVariant( enc3, 1));
  rec4 = SNetRecCreate( REC_data, SNetTencGetVariant( enc4, 1));
  rec5 = SNetRecCreate( REC_data, SNetTencGetVariant( enc5, 1));
  rec6 = SNetRecCreate( REC_data, SNetTencGetVariant( enc6, 1));



  F_x_1 = SNetMemAlloc( sizeof( int));
  F_x_2 = SNetMemAlloc( sizeof( int));
  F_x_3 = SNetMemAlloc( sizeof( int));
  F_x_4 = SNetMemAlloc( sizeof( int));
  F_x_5 = SNetMemAlloc( sizeof( int));
  F_x_6 = SNetMemAlloc( sizeof( int));

  F_z_1 = SNetMemAlloc( sizeof( int));
  F_r_1 = SNetMemAlloc( sizeof( int));
  F_r_2 = SNetMemAlloc( sizeof( int));
  F_r_3 = SNetMemAlloc( sizeof( int));
  F_r_4 = SNetMemAlloc( sizeof( int));
  F_r_5 = SNetMemAlloc( sizeof( int));
  F_r_6 = SNetMemAlloc( sizeof( int));

  *F_x_1 = 4; 
  *F_r_1 = 1;

  *F_x_2 = 2;
  *F_r_2 = 1;
  
  *F_x_3 = 11;
  *F_r_3 = 1;

  *F_x_4 = 7;
  *F_r_4 = 1;
  
  *F_x_5 = 4;
  *F_r_5 = 1;
  
  *F_x_6 = 10;
  *F_r_6 = 1;

  *F_z_1 = 42;

  field1 = C2SNet_cdataCreate( F_x_1, &myfree, &mycopy);
  field2 = C2SNet_cdataCreate( F_r_1, &myfree, &mycopy);
  field3 = C2SNet_cdataCreate( F_x_3, &myfree, &mycopy);
  field4 = C2SNet_cdataCreate( F_r_3, &myfree, &mycopy);


  SNetRecSetField( rec1, F_x, (void*)field1);
  SNetRecSetField( rec1, F_r, (void*)field2);
  SNetRecSetInterfaceId( rec1, 0);

  SNetRecSetField( rec2, F_x, (void*)field3);
  SNetRecSetField( rec2, F_r, (void*)field4);
  SNetRecSetInterfaceId( rec2, 0);


  /*  SNetRecSetField( rec2, F_x, (void*)F_x_2);
  SNetRecSetField( rec3, F_x, (void*)F_x_3);
  SNetRecSetField( rec4, F_x, (void*)F_x_4);
  SNetRecSetField( rec5, F_x, (void*)F_x_5);
  SNetRecSetField( rec6, F_x, (void*)F_x_6);

  SNetRecSetField( rec1, F_r, (void*)F_r_1);
  SNetRecSetField( rec2, F_r, (void*)F_r_2);
  SNetRecSetField( rec3, F_r, (void*)F_r_3);
  SNetRecSetField( rec4, F_r, (void*)F_r_4);
  SNetRecSetField( rec5, F_r, (void*)F_r_5);
  SNetRecSetField( rec6, F_r, (void*)F_r_6);


  SNetRecAddTag( rec1, T_s);
  SNetRecSetTag( rec1, T_s, 43);
*/
  start_buf = SNetBufCreate( 10);
  //REGISTER_BUFFER( start_buf);
  //SET_BUFFER_NAME( start_buf, "initialBuffer");


  SNetBufPut( start_buf, rec1);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec1);
  printf("\n");

  SNetBufPut( start_buf, rec2);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec2);
  printf("\n");

/*
  SNetBufPut( start_buf, rec2);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec2);
  printf("\n"); 

  SNetBufPut( start_buf, rec3);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec3);
  printf("\n");

  SNetBufPut( start_buf, rec4);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec4);
  printf("\n");
  
  SNetBufPut( start_buf, rec5);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec5);
  printf("\n");
  
  SNetBufPut( start_buf, rec6);
  printf("\nPut record to Buffer:");
  PRINT_RECORD( rec6);
  printf("\n");
*/
  printf("\n\n\n");

  res_buf = SNet__factorial___factorial( start_buf);


  printf("\n  >>> Wait,  then press a key to send a termination record. <<<\n");
  getc( stdin);
  SNetBufPut( start_buf, SNetRecCreate( REC_terminate));


  
  printf("\nPress any key to see resulting record\n");
  getc( stdin);

  i = BUFFER_SIZE-SNetGetSpaceLeft( res_buf);
  printf("\n\nResulting Buffer contains %d records.\n", i);
    for( j=0; j<i; j++) {
    printf("\nRecord %d:", j);
    resrec = SNetBufGet( res_buf);
    if( SNetRecGetDescriptor( resrec) == REC_data) {
     PRINT_RECORD_WITH_FREE( resrec);  
     printf("\n");
    }
    else {
      printf("\n - Control Record, Type: %d\n", SNetRecGetDescriptor( resrec));
    }
   // SNetRecDestroy( resrec);
  }
  
  SNetBufDestroy( start_buf);
  printf("\n\nPress any key to exit.\n");
  getc( stdin);
  printf("\nEnd.\n"); 

 

  return( 0);
}
