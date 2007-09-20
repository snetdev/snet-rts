/*
 * - Expressions ---
 * 
 *
 *
 */


#ifndef EXPRESSION_H
#define EXPRESSION_H


#include <bool.h>
#include <stdarg.h>

#include <record.h>

typedef struct expression snet_expr_t;
typedef struct expression_list snet_expr_list_t;


/* *** */
extern snet_expr_t *SNetEconsti( int val);
extern snet_expr_t *SNetEconstb( bool val);
extern snet_expr_t *SNetEtag( int id);
extern snet_expr_t *SNetEbtag( int id);


/* *** */
extern snet_expr_t *SNetEadd( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEsub( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEmul( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEdiv( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEmod( snet_expr_t *a, snet_expr_t *b);


/* *** */
extern snet_expr_t *SNetEabs( snet_expr_t *a);
extern snet_expr_t *SNetEmin( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEmax( snet_expr_t *a, snet_expr_t *b);


/* *** */
extern snet_expr_t *SNetEeq( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEne( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEgt( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEge( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetElt( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEle( snet_expr_t *a, snet_expr_t *b);


/* *** */
extern snet_expr_t *SNetEnot( snet_expr_t *a);
extern snet_expr_t *SNetEand( snet_expr_t *a, snet_expr_t *b);
extern snet_expr_t *SNetEor( snet_expr_t *a, snet_expr_t *b);


/* *** */
extern snet_expr_t *SNetEcond( snet_expr_t *a, snet_expr_t *b, snet_expr_t *c);


/* *** */
extern int SNetEevaluateInt( snet_expr_t *expr, snet_record_t *rec); 
extern bool SNetEevaluateBool( snet_expr_t *expr, snet_record_t *rec); 





/* *** */
extern snet_expr_list_t *SNetEcreateList( int num, ...);


/* *** */
extern int SNetElistGetNum( snet_expr_list_t *l);
extern snet_expr_t *SNetEgetExpr( snet_expr_list_t *l, int num);
#endif
