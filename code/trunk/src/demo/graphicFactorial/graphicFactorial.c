

#include <graphics.h>
#include <bool.h>
#include <memfun.h>

#include "graphicFactorial.h"
#include "boxfun.h"

#define NUM_NAMES 14
#define MAX_NAME_LEN 10



extern char **names;
/*char names[NUM_NAMES][MAX_NAME_LEN] = {
  "F_x",
	"F_r",
  "F_xx",
  "F_rr",
  "F_one",
  "F_p",
  "T_T",
  "T_F",
  "T_out",
  "T_ofl",
  "T_stop",
  "F_z",
  "T_s",
  "BT_tst"
};
*/
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
      SNetTencCreateTypeEncodingList( 1,
        SNetTencTypeEncode( 1,
          SNetTencVariantEncode( 
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)))),
      NULL,
      SNetCreateFilterInstructionSetList( 1,
        SNetCreateFilterInstructionSet( 1,
          SNetCreateFilterInstruction( FLT_strip_tag, T_F))));

  return( outbuf);
}


snet_buffer_t *graphicStripF( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, (char**)names, "After Filter StripF");

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
      SNetTencCreateTypeEncodingList( 1,
        SNetTencTypeEncode( 1,
          SNetTencVariantEncode( 
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 1, T_stop),
            SNetTencCreateVector( 0)))),
      NULL,
      SNetCreateFilterInstructionSetList( 1,
        SNetCreateFilterInstructionSet( 1,
          SNetCreateFilterInstruction( FLT_rename_tag, T_T, T_stop))));
  
  
  
  /*  int *instr;

  instr = SNetMemAlloc( 4 * sizeof( int));
  instr[0] = -5; instr[1] = T_T; instr[2] = T_stop; instr[3] = -10;

  outbuf = SNetFilter( inbuf, instr);
*/
  return( outbuf);
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

  outbuf = SNetGraphicalBox( inbuf, 0, (char**)names, "After Box Dupl");

  return( outbuf);
}

snet_buffer_t *duplbox( snet_buffer_t *inbuf) {
  snet_buffer_t *outbuf;
  outbuf = SNetSerial( inbuf, &boxboxdupl, &graphicdupl);
  return( outbuf);
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
                         SNetTencCreateVector( 0))),NULL);

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
      SNetTencCreateTypeEncodingList( 1,
        SNetTencTypeEncode( 1,
          SNetTencVariantEncode( 
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)))),
      NULL,
      SNetCreateFilterInstructionSetList( 1,
        SNetCreateFilterInstructionSet( 1,
          SNetCreateFilterInstruction( FLT_strip_tag, T_ofl))));
  
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
                            SNetTencCreateVector( 0))),NULL,
      &SYNC_a_b, &starsync2);

  return( outbuf);
}

snet_buffer_t *starsync( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
 
  outbuf =  SNetStar( inbuf,                         SNetTencTypeEncode( 1,
                          SNetTencVariantEncode(
                            SNetTencCreateVector( 2, F_xx, F_rr),
                            SNetTencCreateVector( 0),
                            SNetTencCreateVector( 0))),NULL,
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
      SNetTencCreateTypeEncodingList( 1,
        SNetTencTypeEncode( 1,
          SNetTencVariantEncode( 
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)))),
      NULL,
      SNetCreateFilterInstructionSetList(1,
        SNetCreateFilterInstructionSet( 1,
          SNetCreateFilterInstruction( FLT_strip_tag, T_out))));

  
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
      SNetTencCreateTypeEncodingList( 1,
        SNetTencTypeEncode( 1,
          SNetTencVariantEncode( 
            SNetTencCreateVector( 2, F_x, F_r),
            SNetTencCreateVector( 0),
            SNetTencCreateVector( 0)))),
      NULL,
      SNetCreateFilterInstructionSetList( 1,
        SNetCreateFilterInstructionSet( 2,
          SNetCreateFilterInstruction( FLT_rename_field, F_xx, F_x),
          SNetCreateFilterInstruction( FLT_rename_field, F_rr, F_r))));

  
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

  outbuf = SNetGraphicalBox( inbuf, 0, (char**)names, "After Filter xx->x, rr->r");

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

  outbuf = SNetSerial( inbuf, &stripF, &boxboxdupl);

  return( outbuf);
}

snet_buffer_t *graphicBox1( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, (char**)names, "After the SyncStar");

  return( outbuf);
}


snet_buffer_t *SER_syncstar_filter_graph( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_syncstar_filter, &graphicBox1);

  return( outbuf);
}


snet_buffer_t *SER_sync_filterAB( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_syncstar_filter_graph, &filterAasX_BasR);

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

  outbuf = SNetGraphicalBox( inbuf, 0, (char**)names, "Before the SyncStar");

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
                            SNetTencCreateVector( 0))),NULL,
      &SER_predicate_compute, &starnet2);

  return( outbuf);
}

snet_buffer_t *starnet( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf =  SNetStar( inbuf,                         SNetTencTypeEncode( 1,
                          SNetTencVariantEncode(
                            SNetTencCreateVector( 0),
                            SNetTencCreateVector( 1, T_stop),
                            SNetTencCreateVector( 0))),NULL,
      &SER_predicate_compute, &starnet2);

  return( outbuf);
}

snet_buffer_t *graphicBox3( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf; 

  outbuf = SNetGraphicalBox( inbuf, 0, (char**)names, "Insert to Net");

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
      SNetTencCreateTypeEncodingList( 1, SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0)))),
      NULL,
      SNetCreateFilterInstructionSetList( 1,
        SNetCreateFilterInstructionSet( 2,
          SNetCreateFilterInstruction( FLT_strip_tag, T_stop),
          SNetCreateFilterInstruction( FLT_strip_field, F_x))));
  
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

  outbuf = SNetGraphicalBox( inbuf, 0, (char**)names, "Out of Net");

  return( outbuf);
}


snet_buffer_t *SER_graph_net( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

//  outbuf = SNetSerial( inbuf, &SER_dupl_comp, &SER_sync_filterAB);
  outbuf = SNetSerial( inbuf, &graphicBox3, &SER_starnet_filter);


  return( outbuf);
}



extern snet_buffer_t *SER_net_graph( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

//  outbuf = SNetSerial( inbuf, &SER_dupl_comp, &SER_sync_filterAB);
  outbuf = SNetSerial( inbuf, &SER_graph_net, &graphicBox4);


  return( outbuf);
}


