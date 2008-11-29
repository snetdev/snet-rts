/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : typeencode.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_TYPEENCODE_H
#define __SVPSNETGWRT_TYPEENCODE_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef struct vector             snet_vector_t;

/*---*/

typedef struct type_encoding      snet_typeencoding_t;
typedef struct type_encoding_list snet_typeencoding_list_t;

/*---*/

typedef struct variantencoding    snet_variantencoding_t;

/*---*/

typedef struct box_sign           snet_box_sign_t;

/*---*/

typedef struct patternencoding    snet_patternencoding_t;
typedef struct patternset         snet_patternset_t;


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern snet_vector_t*
SNetTencCreateVector(int num, ...);

/*----------------------------------------------------------------------------*/

extern snet_variantencoding_t*
SNetTencVariantEncode(
    snet_vector_t *field_v, snet_vector_t *tag_v, snet_vector_t *btag_v);

/*----------------------------------------------------------------------------*/

extern snet_typeencoding_t*
SNetTencTypeEncode(int num, ...);

extern snet_typeencoding_list_t*
SNetTencCreateTypeEncodingList(int num, ...);
 
extern snet_typeencoding_list_t*
SNetTencCreateTypeEncodingListFromArray(int num, snet_typeencoding_t **t);

/*----------------------------------------------------------------------------*/

extern snet_box_sign_t*
SNetTencBoxSignEncode(snet_typeencoding_t *t, ...);

/*----------------------------------------------------------------------------*/

extern snet_patternencoding_t*
SNetTencPatternEncode(int num_tags, int num_fields, ...);

#endif // __SVPSNETGWRT_TYPEENCODE_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

