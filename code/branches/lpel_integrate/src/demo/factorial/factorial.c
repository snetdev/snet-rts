/*
 * Comment
 *
 *
 */

 
#include <bool.h>
#include <memfun.h>
#include <snetentities.h>

#include "factorial.h"
#include "boxfun.h"


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

#ifdef FILTER_VERSION_2
  outbuf = SNetFilter( inbuf,
                       SNetTencTypeEncode( 1,
                         SNetTencVariantEncode( 
                           SNetTencCreateVector( 0),
                           SNetTencCreateVector( 1, T_F),
                           SNetTencCreateVector( 0))),
                       NULL,
                       NULL);
#else
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_F),
          SNetTencCreateVector( 0))),
      SNetTencCreateTypeEncodingList( 1, SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0)))), NULL,
      SNetCreateFilterInstructionSetList( 1, SNetCreateFilterInstructionSet( 1,
        SNetCreateFilterInstruction( FLT_strip_tag, T_F))));
#endif  
  return( outbuf);
}

snet_buffer_t *filterTasSTOP( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;
#ifdef FILTER_VERSION_2
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_T),
          SNetTencCreateVector( 0))),
        NULL,
        SNetCreateFilterInstructionSetList( 1,
          SNetCreateFilterInstructionSet( 1,
            SNetCreateFilterInstruction( 
              snet_tag, T_stop, SNetEtag( T_T)))));

#else
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_T),
          SNetTencCreateVector( 0))),
      SNetTencCreateTypeEncodingList( 1, SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_stop),
          SNetTencCreateVector( 0)))), NULL,
      SNetCreateFilterInstructionSetList( 1, SNetCreateFilterInstructionSet( 1,
        SNetCreateFilterInstruction( FLT_rename_tag, T_T, T_stop))));
#endif
  
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
                         SNetTencCreateVector( 0))), 
                     SNetEcreateList( 2,
                       SNetEconstb( true),
                       SNetEconstb( true)));

  return( outbuf);
}


snet_buffer_t *filterOFL( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;
#ifdef  FILTER_VERSION_2
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_ofl),
          SNetTencCreateVector( 0))),
        NULL,
        NULL);
#else
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_ofl),
          SNetTencCreateVector( 0))),
      SNetTencCreateTypeEncodingList( 1, SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0)))), NULL,
      SNetCreateFilterInstructionSetList( 1, SNetCreateFilterInstructionSet( 1,
        SNetCreateFilterInstruction( FLT_strip_tag, T_ofl))));
#endif  

  return( outbuf);
}



snet_buffer_t *SER_sync_filter( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SYNC_a_b, &filterOFL);

  return( outbuf);
}



snet_buffer_t *starsync2( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
  outbuf =  SNetStarDetIncarnate( inbuf, 
                               SNetTencTypeEncode( 1,
                                 SNetTencVariantEncode(
                                  SNetTencCreateVector( 2, F_xx, F_rr),
                                  SNetTencCreateVector( 0),
                                  SNetTencCreateVector( 0))),
                               SNetEcreateList( 1, SNetEconstb( true)),
                               &SYNC_a_b, &starsync2);


  return( outbuf);
}

snet_buffer_t *starsync( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;
   outbuf =  SNetStarDet( inbuf, 
                       SNetTencTypeEncode( 1,
                        SNetTencVariantEncode(
                           SNetTencCreateVector( 2, F_xx, F_rr),
                           SNetTencCreateVector( 0),
                           SNetTencCreateVector( 0))),
                       SNetEcreateList( 1, SNetEconstb( true)),
                       &SYNC_a_b, &starsync2);


  return( outbuf);
}

snet_buffer_t *filterOUT( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;
#ifdef FILTER_VERSION_2
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_out),
          SNetTencCreateVector( 0))),
        NULL,
        NULL);
#else
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 1, T_out),
          SNetTencCreateVector( 0))),
      SNetTencCreateTypeEncodingList( 1, SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0)))), NULL,
      SNetCreateFilterInstructionSetList( 1, SNetCreateFilterInstructionSet( 1,
        SNetCreateFilterInstruction( FLT_strip_tag, T_out))));
#endif
  
  return( outbuf);
}

snet_buffer_t *SER_syncstar_filter( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &starsync, &filterOUT);

  return( outbuf);
}






snet_buffer_t *filterAasX_BasR( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;
#ifdef FILTER_VERSION_2
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 2, F_xx, F_rr),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0))),
        NULL,
        SNetCreateFilterInstructionSetList( 1,
          SNetCreateFilterInstructionSet( 2,
            SNetCreateFilterInstruction( snet_field, F_x, F_xx),
            SNetCreateFilterInstruction( snet_field, F_r, F_rr))));
#else
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 2, F_xx, F_rr),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0))),
      SNetTencCreateTypeEncodingList( 1, SNetTencTypeEncode( 1,
        SNetTencVariantEncode( 
          SNetTencCreateVector( 2, F_x, F_r),
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0)))), NULL,
      SNetCreateFilterInstructionSetList( 1, SNetCreateFilterInstructionSet( 2,
        SNetCreateFilterInstruction( FLT_rename_field, F_xx, F_x),
        SNetCreateFilterInstruction( FLT_rename_field, F_rr, F_r))));
#endif
  
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




snet_buffer_t *SER_sync_filterAB( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_syncstar_filter, &filterAasX_BasR);

  return( outbuf);
}


snet_buffer_t *SER_dupl_comp( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_filterF_dupl, &PAR_comp);

  return( outbuf);
}


snet_buffer_t *SER_case_false( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &SER_dupl_comp, &SER_sync_filterAB);

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
  outbuf =  SNetStarDetIncarnate( inbuf, 
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
  outbuf =  SNetStarDet( inbuf, 
                         SNetTencTypeEncode( 1,
                          SNetTencVariantEncode(
                            SNetTencCreateVector( 0),
                            SNetTencCreateVector( 1, T_stop),
                            SNetTencCreateVector( 0))),NULL,
                         &SER_predicate_compute, &starnet2);


  return( outbuf);
}


snet_buffer_t *filterX( snet_buffer_t *inbuf) {
  
  snet_buffer_t *outbuf;
#ifdef FILTER_VERSION_2
  outbuf = SNetFilter( inbuf,
      SNetTencTypeEncode( 1,
        SNetTencVariantEncode(
          SNetTencCreateVector( 1, F_x),
          SNetTencCreateVector( 1, T_stop),
          SNetTencCreateVector( 0))),
        NULL,
        NULL);
#else  
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
          SNetTencCreateVector( 0)))), NULL,
      SNetCreateFilterInstructionSetList( 1, SNetCreateFilterInstructionSet( 2,
        SNetCreateFilterInstruction( FLT_strip_tag, T_stop),
        SNetCreateFilterInstruction( FLT_strip_field, F_x))));
#endif  

  return( outbuf);
}

snet_buffer_t *SER_starnet_filter( snet_buffer_t *inbuf) {

  snet_buffer_t *outbuf;

  outbuf = SNetSerial( inbuf, &starnet, &filterX);

  return( outbuf);
}








