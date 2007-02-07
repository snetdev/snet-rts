
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#include <graphics.h>
#include <buffer.h>
#include <record.h>
#include <typeencode.h>
#include <memfun.h>
#include <constants.h>


#include <snetentities.h>




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
    


#define F_x     0
#define F_r     1
#define F_xx     2
#define F_rr     3
#define F_one   4
#define F_p     5

#define T_T     6
#define T_F     7
#define T_out   8
#define T_ofl   9
#define T_stop 10
#define F_z    11
#define T_s    12
#define BT_tst 13 
#define NUM_NAMES 14

char **names;


snet_handle_t *leq1( snet_handle_t *hnd, void *field_x) {

  bool *p;
  int *f_x;
  

  f_x = SNetMemAlloc( sizeof( int));
  *f_x = *(int*)field_x; 

  p = SNetMemAlloc( sizeof( bool));

  if( *f_x <= 1) {
    *p = true;
  }
  else {
    *p = false;
  }
  
  SNetOutRaw( hnd, 1, (void*)f_x, (void*)p);

  return( hnd);
}


void boxleq1( snet_handle_t *hnd) {

  snet_record_t *rec;
  void *field_x;
  rec = SNetHndGetRecord( hnd);

  field_x = SNetRecTakeField( rec, F_x);

  leq1( hnd, field_x);
}

snet_buffer_t *boxboxleq1( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
  snet_typeencoding_t *tenc;

  tenc = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 2, F_x, F_p),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)));

  outbuf = SNetBox( inbuf, &boxleq1, tenc);

  return( outbuf);
}




snet_handle_t *condif( snet_handle_t *hnd, void *field_p) {

   bool *p = (bool*) field_p;

  if( *p) {
    SNetOutRaw( hnd, 1, 0);
  }
  else {
    SNetOutRaw( hnd, 2, 0);
  }

  SNetMemFree( p);

  return( hnd);
}


void boxcondif( snet_handle_t *hnd) {

   snet_record_t *rec;
  void *field_p;

  rec = SNetHndGetRecord( hnd);
  field_p = SNetRecTakeField( rec, F_p);

  condif( hnd, field_p);
}

snet_buffer_t *boxboxcondif( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
  snet_typeencoding_t *tenc;

  tenc = SNetTencTypeEncode( 2,
          SNetTencVariantEncode( 
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 1, T_T),
            SNetTencCreateVector( 0)),
          SNetTencVariantEncode( 
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 1, T_F),
            SNetTencCreateVector( 0)));

  outbuf = SNetBox( inbuf, &boxcondif, tenc);

  return( outbuf);
}


snet_buffer_t *stripF( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_F),
          SNetTencCreateVector( 0))),
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0))),
      SNetCreateFilterInstructionSet( 1,
        SNetCreateFilterInstruction( FLT_strip_tag, T_F)));

  
  
  /*  int *instr;

  instr = SNetMemAlloc( 3 * sizeof( int));
  instr[0] = -2; instr[1] = T_F; instr[2] = -10; 
 
  outbuf = SNetFilter( inbuf, instr);
*/
  return( outbuf);
}


snet_buffer_t *graphicStripF( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, names, "After Filter StripF");

  return( outbuf);

}

snet_buffer_t *graphicStripFBox( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &stripF, &graphicStripF);
  return( outbuf);
}
snet_buffer_t *filterTasSTOP( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;

  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_T),
          SNetTencCreateVector( 0))),
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_stop),
          SNetTencCreateVector( 0))),
      SNetCreateFilterInstructionSet( 1,
        SNetCreateFilterInstruction( FLT_rename_tag, T_T, T_stop)));
  
  
  
  /*  int *instr;

  instr = SNetMemAlloc( 4 * sizeof( int));
  instr[0] = -5; instr[1] = T_T; instr[2] = T_stop; instr[3] = -10;

  outbuf = SNetFilter( inbuf, instr);
*/
  return( outbuf);
}


snet_handle_t *dupl( snet_handle_t *hnd, void *field_x, void *field_r) {

  int *f_x1, *f_x2, *f_r;
  
  f_x1 = SNetMemAlloc( sizeof( int));
  *f_x1 = *(int*)field_x;

  f_x2 = SNetMemAlloc( sizeof( int));
  *f_x2 = *(int*)field_x;
  
  f_r = SNetMemAlloc( sizeof( int));
  *f_r = *(int*)field_r;


  SNetOutRaw( hnd, 1, (void*)f_x1);
  SNetOutRaw( hnd, 2, (void*)f_x2, (void*)f_r);

  return( hnd);
}


void boxdupl( snet_handle_t *hnd) {

  snet_record_t *rec;
  void *field_x, *field_r;

  rec = SNetHndGetRecord( hnd);
  field_x = SNetRecTakeField( rec, F_x);
  field_r = SNetRecTakeField( rec, F_r);

  dupl( hnd, field_x, field_r);
}

snet_buffer_t *boxboxdupl( snet_buffer_t *inbuf) {

  snet_typeencoding_t *tenc;
  snet_buffer_t *outbuf;

  tenc = SNetTencTypeEncode( 2, 
          SNetTencVariantEncode(
            SNetTencCreateVector( 1, F_x),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)),
          SNetTencVariantEncode(
            SNetTencCreateVector( 2, F_x, F_r),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)));

  outbuf = SNetBox( inbuf, &boxdupl, tenc);

  return( outbuf);
}


snet_buffer_t *graphicdupl( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, names, "After Box Dupl");

  return( outbuf);
}

snet_buffer_t *duplbox( snet_buffer_t *inbuf) {
  snet_buffer_t *outbuf;
  outbuf = SNetSerial( inbuf, &boxboxdupl, &graphicdupl);
  return( outbuf);
}

snet_handle_t *sub( snet_handle_t *hnd, void *field_x) {

  int *a;

  a = SNetMemAlloc( sizeof( int));

  *a = *((int*)field_x) - 1;
  SNetOutRaw( hnd, 1, (void*)a);

  return( hnd);
}

void boxsub( snet_handle_t *hnd) {

  snet_record_t *rec;
  void *field_x;

  rec = SNetHndGetRecord( hnd);
  field_x = SNetRecTakeField( rec, F_x);

  sub( hnd, field_x);
}

snet_buffer_t *boxboxsub( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
  snet_typeencoding_t *tenc;

  tenc = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 1, F_xx),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)));

  outbuf = SNetBox( inbuf, &boxsub, tenc);


  return( outbuf);
}

// ----------------------------------------------------------------------------


snet_handle_t *mult( snet_handle_t *hnd, void *field_x, void *field_r) {

  int *b;

  b = SNetMemAlloc( sizeof( int));

  *b = *((int*)field_x) * *((int*)field_r);
  SNetOutRaw( hnd, 1, (void*)b);

  return( hnd);
}

void boxmult( snet_handle_t *hnd) {

  snet_record_t *rec;
  void *field_x, *field_r;

  rec = SNetHndGetRecord( hnd);
  field_x = SNetRecTakeField( rec, F_x);
  field_r = SNetRecTakeField( rec, F_r);

  mult( hnd, field_x, field_r);
}

snet_buffer_t *boxboxmult( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
  snet_typeencoding_t *tenc;

  tenc = SNetTencTypeEncode( 1, 
          SNetTencVariantEncode( 
            SNetTencCreateVector( 1, F_rr),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)));

  outbuf = SNetBox( inbuf, &boxmult, tenc);

  return( outbuf);
}



snet_buffer_t *SYNC_a_b( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
  outbuf = SNetSync( inbuf, 
                     SNetTencTypeEncode( 1,
                       SNetTencVariantEncode( 
                         SNetTencCreateVector( 2, F_xx, F_rr),
                         SNetTencCreateVector( 0),
                         SNetTencCreateVector( 0))),
                     SNetTencTypeEncode( 2,
                       SNetTencVariantEncode( 
                         SNetTencCreateVector( 1, F_xx),
                         SNetTencCreateVector( 0),
                         SNetTencCreateVector( 0)),
                       SNetTencVariantEncode(
                         SNetTencCreateVector( 1, F_rr),
                         SNetTencCreateVector( 0),
                         SNetTencCreateVector( 0))));

  return( outbuf);
}


snet_buffer_t *filterOFL( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;

  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_ofl),
          SNetTencCreateVector( 0))),
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0))),
      SNetCreateFilterInstructionSet( 1,
        SNetCreateFilterInstruction( FLT_strip_tag, T_ofl)));
  
/*  int *instr;

  instr = SNetMemAlloc( 3 * sizeof( int));
  instr[0] = -2; instr[1] = T_ofl; 
  instr[2] = -10;

  outbuf =  SNetFilter( inbuf, instr);*/
  return( outbuf);
}



snet_buffer_t *SER_sync_filter( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SYNC_a_b, &filterOFL);

  return( outbuf);
}



snet_buffer_t *starsync2( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
  outbuf =  SNetStarIncarnate( inbuf,                         SNetTencTypeEncode( 1,
                          SNetTencVariantEncode(
                            SNetTencCreateVector( 2, F_xx, F_rr),
                            SNetTencCreateVector( 0),
                            SNetTencCreateVector( 0))),
      &SYNC_a_b, &starsync2);

  return( outbuf);
}

snet_buffer_t *starsync( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
 
  outbuf =  SNetStar( inbuf,                         SNetTencTypeEncode( 1,
                          SNetTencVariantEncode(
                            SNetTencCreateVector( 2, F_xx, F_rr),
                            SNetTencCreateVector( 0),
                            SNetTencCreateVector( 0))),
      &SYNC_a_b, &starsync2);

  return( outbuf);
}

snet_buffer_t *filterOUT( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_out),
          SNetTencCreateVector( 0))),
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0))),
      SNetCreateFilterInstructionSet( 1,
        SNetCreateFilterInstruction( FLT_strip_tag, T_out)));

  
  /*  int *instr;

  instr = SNetMemAlloc( 3 * sizeof( int));
  instr[0] = -2; instr[1] = T_out; 
  instr[2] = -10;

  outbuf = SNetFilter( inbuf, instr);
*/
  return( outbuf);
}

snet_buffer_t *SER_syncstar_filter( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &starsync, &filterOUT);

  return( outbuf);
}






snet_buffer_t *filterAasX_BasR( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 2, F_xx, F_rr),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0))),
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 2, F_x, F_r),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0))),
      SNetCreateFilterInstructionSet( 2,
        SNetCreateFilterInstruction( FLT_rename_field, F_xx, F_x),
        SNetCreateFilterInstruction( FLT_rename_field, F_rr, F_r)));

  
  /*  int *instr;

  instr = SNetMemAlloc( 7 * sizeof( int));
  instr[0] = -4; instr[1] = F_a; instr[2] = F_x;
  instr[3] = -4; instr[4] = F_rr; instr[5] = F_r;
  instr[6] = -10;

  outbuf = SNetFilter( inbuf, instr);
*/
  return( outbuf);
}



snet_buffer_t *graphicAasX( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, names, "After Filter xx->x, rr->r");

  return( outbuf);

}


snet_buffer_t *AasX( snet_buffer_t *inbuf) {
  snet_buffer_t *outbuf;
  
  outbuf = SNetSerial( inbuf, &filterAasX_BasR, &graphicAasX);

  return( outbuf);
}




snet_buffer_t *PAR_comp( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

    outbuf = SNetParallel( inbuf, 
                        SNetTencTypeEncode( 2,
                           SNetTencVariantEncode( 
                             SNetTencCreateVector( 1, F_x),
                             SNetTencCreateVector( 0),
                             SNetTencCreateVector( 0)),
                           SNetTencVariantEncode(
                             SNetTencCreateVector( 2, F_x, F_r),
                             SNetTencCreateVector( 0),
                             SNetTencCreateVector( 0))),
                           &boxboxsub, &boxboxmult);
 
  return( outbuf);
}


snet_buffer_t *SER_filterF_dupl( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &graphicStripFBox, &duplbox);

  return( outbuf);
}

snet_buffer_t *graphicBox1( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, names, "After the SyncStar");

  return( outbuf);
}


snet_buffer_t *SER_syncstar_filter_graph( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_syncstar_filter, &graphicBox1);

  return( outbuf);
}


snet_buffer_t *SER_sync_filterAB( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_syncstar_filter_graph, &AasX);

//  outbuf = SNetSerial( inbuf, &SER_syncstar_filter, &filterAasX_BasR);

  return( outbuf);
}


snet_buffer_t *SER_dupl_comp( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_filterF_dupl, &PAR_comp);

  return( outbuf);
}

snet_buffer_t *graphicBox2( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, names, "Before the SyncStar");

  return( outbuf);
}

snet_buffer_t *SER_graphic_sync( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &graphicBox2, &SER_sync_filterAB);

  return( outbuf);
}
snet_buffer_t *SER_case_false( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

//  outbuf = SNetSerial( inbuf, &SER_dupl_comp, &SER_sync_filterAB);
  outbuf = SNetSerial( inbuf, &SER_dupl_comp, &SER_graphic_sync);


  return( outbuf);
}





snet_buffer_t *PAR_TorF( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

    outbuf = SNetParallel( inbuf, 
                        SNetTencTypeEncode( 2,
                           SNetTencVariantEncode( 
                             SNetTencCreateVector( 0),
                             SNetTencCreateVector( 1, T_T),
                             SNetTencCreateVector( 0)),
                           SNetTencVariantEncode(
                             SNetTencCreateVector( 0),
                             SNetTencCreateVector( 1, T_F),
                             SNetTencCreateVector( 0))),
                          &filterTasSTOP, &SER_case_false);
 
  return( outbuf);
}

snet_buffer_t *SER_predicate( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &boxboxleq1, &boxboxcondif);

  return( outbuf);
}


snet_buffer_t *SER_predicate_compute( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_predicate, &PAR_TorF);

  return( outbuf);
}


snet_buffer_t *starnet2( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf =  SNetStarIncarnate( inbuf, 
      SNetTencTypeEncode( 1,
                          SNetTencVariantEncode(
                            SNetTencCreateVector( 0),
                            SNetTencCreateVector( 1, T_stop),
                            SNetTencCreateVector( 0))),
      &SER_predicate_compute, &starnet2);

  return( outbuf);
}

snet_buffer_t *starnet( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf =  SNetStar( inbuf,                         SNetTencTypeEncode( 1,
                          SNetTencVariantEncode(
                            SNetTencCreateVector( 0),
                            SNetTencCreateVector( 1, T_stop),
                            SNetTencCreateVector( 0))),
      &SER_predicate_compute, &starnet2);

  return( outbuf);
}

snet_buffer_t *graphicBox3( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, names, "Insert to Net");

  return( outbuf);

}







snet_buffer_t *filterX( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;
  
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 1, F_x),
          SNetTencCreateVector( 1, T_stop),
          SNetTencCreateVector( 0))),
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0))),
      SNetCreateFilterInstructionSet( 2,
        SNetCreateFilterInstruction( FLT_strip_tag, T_stop),
        SNetCreateFilterInstruction( FLT_strip_field, F_x)));
  
/*  
  instr[0] = -1; instr[1] = F_x; 
  instr[2] = -2; instr[3] = T_stop; instr[4] = -10;

  outbuf = SNetFilter( inbuf, instr);
*/

  return( outbuf);
}

snet_buffer_t *SER_starnet_filter( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &starnet, &filterX);

  return( outbuf);
}




snet_buffer_t *graphicBox4( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, names, "Out of Net");

  return( outbuf);
}


snet_buffer_t *SER_graph_net( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

//  outbuf = SNetSerial( inbuf, &SER_dupl_comp, &SER_sync_filterAB);
  outbuf = SNetSerial( inbuf, &graphicBox3, &SER_starnet_filter);


  return( outbuf);
}



snet_buffer_t *SER_net_graph( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

//  outbuf = SNetSerial( inbuf, &SER_dupl_comp, &SER_sync_filterAB);
  outbuf = SNetSerial( inbuf, &SER_graph_net, &graphicBox4);


  return( outbuf);
}



void initialize() {

  int i, num;


  num = NUM_NAMES;
  names = SNetMemAlloc( num * sizeof( char*));

  for( i=0; i<num; i++) {
    names[i] = SNetMemAlloc( 10 * sizeof( char));
  }
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

//  res_buf = graphicBox1( start_buf);
//  res_buf = outofNetDisplay( SNetGraphicalFeeder( start_buf, names, 13));
//  res_buf = outofNetDisplay( start_buf);
//  res_buf = starnet( start_buf);
//  res_buf = SER_starnet_filter( SNetGraphicalFeeder( start_buf, names, 13));
  res_buf = SER_net_graph(  SNetGraphicalFeeder( start_buf, names, NUM_NAMES));
//  res_buf = starsync( start_buf);



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





