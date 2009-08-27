/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

         * * * * ! SVP4SNetc (S-Net compiler plugin for SVP) ! * * * *
 
                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2009
                         
      -------------------------------------------------------------------

    File Name      : SVP4SNetc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : 

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

#ifndef __SVP4SNETC_H
#define __SVP4SNETC_H

#ifdef __cplusplus
#define BOOL_H
#endif

/*---*/

#include <typeencode.h>
#include <metadata.h>

/*----------------------------------------------------------------------------*/
/**
 * Pointer to functions that generate the "actual"
 * box wrapper generation function.
 */

typedef char* (*snet_genwrapper_fptr_t)(
    char *, snet_input_type_enc_t *, snet_meta_data_enc_t *);

/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

char*
SVP4SNetGenBoxWrapper( 
    char *box_name,
    snet_input_type_enc_t *type,
    snet_meta_data_enc_t  *metadata);

char*
SVP4SNetGenBoxWrapperEx( 
    char *box_name,
    snet_input_type_enc_t *type,
    snet_meta_data_enc_t  *meta_data,
    snet_genwrapper_fptr_t decldef_genwrapper_fun,
    snet_genwrapper_fptr_t callstmnt_genwrapper_fun);

#ifdef __cplusplus
}
#endif
#endif // __SVP4SNETC_H

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

