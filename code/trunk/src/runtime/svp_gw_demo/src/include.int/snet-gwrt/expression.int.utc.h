/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : expression.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_EXPRESSION_INT_H
#define __SVPSNETGWRT_EXPRESSION_INT_H

#include "expression.utc.h"

/*---*/

#include "common.int.utc.h"
#include "record.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern int 
SNetEEvaluateInt(const snet_expr_t *expr, const snet_record_t *rec); 

extern bool
SNetEEvaluateBool(const snet_expr_t *expr, const snet_record_t *rec); 

#endif // __SVPSNETGWRT_EXPRESSION_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

