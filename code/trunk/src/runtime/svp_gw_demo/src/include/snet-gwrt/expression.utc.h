/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : expression.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_EXPRESSION_H
#define __SVPSNETGWRT_EXPRESSION_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef struct expr      snet_expr_t;
typedef struct expr_list snet_expr_list_t;

/*---*/

typedef enum {
    EXPR_TYPE_CONSTI,
    EXPR_TYPE_CONSTB,
    EXPR_TYPE_TAG,
    EXPR_TYPE_BTAG,
    EXPR_TYPE_UNARY,
    EXPR_TYPE_BINARY,
    EXPR_TYPE_COND

} snet_expr_type_t;

typedef enum {
    EXPR_OP_NONE,
    EXPR_OP_ABS,
    EXPR_OP_MIN,
    EXPR_OP_MAX,
    EXPR_OP_ADD,
    EXPR_OP_MUL,
    EXPR_OP_SUB,
    EXPR_OP_DIV,
    EXPR_OP_MOD,
    EXPR_OP_EQ,
    EXPR_OP_NE,
    EXPR_OP_GT,
    EXPR_OP_GE,
    EXPR_OP_LT,
    EXPR_OP_LE,
    EXPR_OP_AND,
    EXPR_OP_OR,
    EXPR_OP_NOT

} snet_expr_op_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Expression functions */

extern snet_expr_t* SNetEconsti(int val);
extern snet_expr_t* SNetEconstb(bool val);

/*---*/

extern snet_expr_t* SNetEtag(int name);
extern snet_expr_t* SNetEbtag(int name);

/*---*/

extern snet_expr_t*
SNetEadd(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetEsub(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t* 
SNetEmul(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetEdiv(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetEmod(const snet_expr_t *a, const snet_expr_t *b);

/*---*/

extern snet_expr_t*
SNetEabs(const snet_expr_t *a);

extern snet_expr_t*
SNetEmin(const snet_expr_t *a, snet_expr_t *b);

extern snet_expr_t*
SNetEmax(const snet_expr_t *a, snet_expr_t *b);

/*---*/

extern snet_expr_t*
SNetEeq(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetEne(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetEgt(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetEge(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetElt(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetEle(const snet_expr_t *a, const snet_expr_t *b);

/*---*/

extern snet_expr_t*
SNetEnot(const snet_expr_t *a);

extern snet_expr_t*
SNetEor(const snet_expr_t *a, const snet_expr_t *b);

extern snet_expr_t*
SNetEand(const snet_expr_t *a, const snet_expr_t *b);

/*---*/

extern snet_expr_t*
SNetEcond(const snet_expr_t *a, const snet_expr_t *b, const snet_expr_t *c);

/*----------------------------------------------------------------------------*/

extern void
SNetEDestroy(snet_expr_t *expr);

/*---*/

extern snet_expr_type_t
SNetEGetType(const snet_expr_t *expr);

extern snet_expr_op_t
SNetEGetOperator(const snet_expr_t *expr);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Expression list functions */

extern snet_expr_list_t*
SNetEListCreate(unsigned int sz, ...);

/*---*/

extern void SNetEListDestroy(snet_expr_list_t *lst);

/*----------------------------------------------------------------------------*/

extern unsigned int 
SNetEListGetSize(const snet_expr_list_t *lst);

extern const snet_expr_t*
SNetEListGetExpr(const snet_expr_list_t *lst, unsigned int inx);

#endif // __SVPSNETGWRT_EXPRESSION_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

