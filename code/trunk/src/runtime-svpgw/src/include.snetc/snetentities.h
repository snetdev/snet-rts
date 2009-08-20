/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : snetentities.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Header included from the S-Net compiler 
                     generated code as a master header for the
                     runtime.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_SNETC_SNETENTITIES_H
#define __SVPSNETGWRT_SNETC_SNETENTITIES_H

#include "snetgw.utc.h"

/*----------------------------------------------------------------------------*/
/**
 * Workaround for the fact that the SNet compiler
 * uses "char *" for string constants as opposed
 * to "const char *" which depending on the compiler
 * used for the selected SVP platform "annoying"
 * warnings are issued for every single label!!!!
 */
#define char const char

/*----------------------------------------------------------------------------*/

#define field 0
#define tag   1
#define btag  2

/*---*/

#define REC_data      SNET_REC_DESCR_DATA
#define REC_terminate SNET_REC_DESCR_CTRL

#define MODE_textual  SNET_REC_DATA_MODE_TXT
#define MODE_binary   SNET_REC_DATA_MODE_BIN

/*---*/

#define snet_tag      SNET_FILTER_OP_TAG
#define snet_btag     SNET_FILTER_OP_BTAG
#define snet_field    SNET_FILTER_OP_FIELD,
#define create_record SNET_FILTER_OP_CREATE_REC

/*---*/

#define SNetEcreateList      SNetEListCreate
#define SNetTencCreateVector SNetTencVectorCreate
#define SNetGlobalInitialise SNetGlobalInitialize

/*----------------------------------------------------------------------------*/
/**
 * Datatype representing a stream that is
 * used by the S-Net compiler as input for the
 * "entities" functions of the CI. For us this
 * is also "snet_gnode_t". 
 *
 * It is required so that no changes are needed in 
 * the compiler generated code.
 */
typedef snet_gnode_t snet_tl_stream_t;

#endif // __SVPSNETGWRT_SNETC_SNETENTITIES_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

