/*
 * expression.c
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "expression.h"

#include "record.h"
#include "memfun.h"



typedef enum {
  CONSTI,
  CONSTB,
  TAG,
  BTAG,
  ABS,
  MIN,
  MAX,
  ADD,
  MUL,
  SUB,
  DIV,
  MOD,
  EQ,
  NE,
  GT,
  GE,
  LT,
  LE,
  AND,
  OR,
  NOT,
  COND
} snet_expr_type_t;



union snet_expr_content_t {
  snet_expr_t **expr;
  int *ival;
  bool *bval;
} ;

struct expression {
  snet_expr_type_t type;
  union snet_expr_content_t content;
};


struct expression_list {
  int num;
  snet_expr_t **list;
};


static snet_expr_t *CreateExpr( snet_expr_type_t t) {
  snet_expr_t *new;

  new = SNetMemAlloc( sizeof( snet_expr_t));
  new->type = t;
  
  switch( t) {
    case CONSTI:
    case TAG:
    case BTAG:
      new->content.ival = SNetMemAlloc( sizeof( int));
      break;
    case CONSTB:
      new->content.bval = SNetMemAlloc( sizeof( bool));
      break;
    case COND:
      new->content.expr = SNetMemAlloc( 3 * sizeof( snet_expr_t*));
      break;
    case ABS:
    case NOT:
      new->content.expr = SNetMemAlloc( sizeof( snet_expr_t*));
      break;
    default:
      new->content.expr = SNetMemAlloc( 2 * sizeof( snet_expr_t*));
  }

  return( new);
}

void SNetEdestroyExpr( snet_expr_t *expr) {

  int i;

  switch( expr->type) {
    case CONSTI:
    case TAG:
    case BTAG:
      SNetMemFree( expr->content.ival);
      break;
    case CONSTB:
      SNetMemFree( expr->content.bval);
      break;
    case COND:
      for( i=0; i<3; i++) {
        SNetMemFree( expr->content.expr[i]);
      }
      SNetMemFree(expr->content.expr);
      break;
    case NOT:
    case ABS:
      SNetMemFree( expr->content.expr[0]);
      SNetMemFree(expr->content.expr);
      break;
    default:
      for( i=0; i<2; i++) {
        SNetMemFree( expr->content.expr[i]);
      }
      SNetMemFree(expr->content.expr);
  }
  SNetMemFree( expr);
}


extern snet_expr_t *SNetEconsti( int val) {
  snet_expr_t *new;

  new = CreateExpr( CONSTI);
  *new->content.ival = val;
  
 return( new); 
}

extern snet_expr_t *SNetEconstb( bool val) {
  snet_expr_t *new;

  new = CreateExpr( CONSTB);
  *new->content.bval = val;
  
 return( new); 
}

static snet_expr_t *CreateTagExpr( snet_expr_type_t t, int id) {
  snet_expr_t *new;

  new = CreateExpr( t);
  *new->content.ival = id;
  
 return( new); 
}

extern snet_expr_t *SNetEtag( int id) {
  return( CreateTagExpr( TAG, id)); 
}

extern snet_expr_t *SNetEbtag( int id) {
  return( CreateTagExpr( BTAG, id)); 
}

static snet_expr_t *CreateMonOp( snet_expr_type_t t, snet_expr_t *a) {
  snet_expr_t *new;

  new = CreateExpr( t);
  new->content.expr[0] = a;

  return( new);
}

static snet_expr_t *CreateBinOp( snet_expr_type_t t, snet_expr_t *a, 
                                                     snet_expr_t *b) {
  snet_expr_t *new;

  new = CreateExpr( t);
  new->content.expr[0] = a;
  new->content.expr[1] = b;

  return( new);
}

static snet_expr_t *CreateTriOp( snet_expr_type_t t, snet_expr_t *a,
                                                     snet_expr_t *b, 
                                                     snet_expr_t *c) {
  snet_expr_t *new;

  new = CreateExpr( t);
  new->content.expr[0] = a;
  new->content.expr[1] = b;
  new->content.expr[2] = c;

  return( new);
}

extern snet_expr_t *SNetEadd( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( ADD, a, b));
}

extern snet_expr_t *SNetEsub( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( SUB, a, b));
}

extern snet_expr_t *SNetEmul( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( MUL, a, b));
}

extern snet_expr_t *SNetEdiv( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( DIV, a, b));
}

extern snet_expr_t *SNetEmod( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( MOD, a, b));
}

extern snet_expr_t *SNetEmin( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( MIN, a, b));
}

extern snet_expr_t *SNetEabs( snet_expr_t *a) {
  return( CreateMonOp( ABS, a));
}

extern snet_expr_t *SNetEmax( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( MAX, a, b));
}

extern snet_expr_t *SNetEeq( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( EQ, a, b));
}

extern snet_expr_t *SNetEne( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( NE, a, b));
}

extern snet_expr_t *SNetEgt( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( GT, a, b));
}

extern snet_expr_t *SNetEge( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( GE, a, b));
}

extern snet_expr_t *SNetElt( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( LT, a, b));
}

extern snet_expr_t *SNetEle( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( LE, a, b));
}

extern snet_expr_t *SNetEand( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( AND, a, b));
}

extern snet_expr_t *SNetEor( snet_expr_t *a, snet_expr_t *b) {
  return( CreateBinOp( OR, a, b));
}

extern snet_expr_t *SNetEnot( snet_expr_t *a) {
  return( CreateMonOp( NOT, a));
}

extern snet_expr_t *SNetEcond( snet_expr_t *a, snet_expr_t *b, snet_expr_t *c) {
  return( CreateTriOp( COND, a, b, c));
}
/*
--------------------------------------------------------------------------------
*/

extern snet_expr_list_t *SNetEcreateList( int num, ...) 
{
  
  int i;
  snet_expr_list_t *lst;
  va_list args;

  lst = SNetMemAlloc( sizeof( snet_expr_list_t));
  lst->num = num;
  lst->list = SNetMemAlloc( num * sizeof( snet_expr_t*));

  va_start( args, num);
  for( i=0; i<num; i++) {
    lst->list[i] = va_arg( args, snet_expr_t*);
  }
  va_end( args);


  return( lst);
}

extern void SNetEdestroyList( snet_expr_list_t *lst) 
{
  
  int i;
  if(lst != NULL) {
    for( i=0; i<lst->num; i++) {
      if(lst->list[i] != NULL) {
	SNetEdestroyExpr(lst->list[i]);
      }
    }
    SNetMemFree(lst->list);
    
    SNetMemFree(lst);
  }
  return;
}

extern int SNetElistGetNumExpressions( snet_expr_list_t *lst) 
{
  if( lst == NULL) {
    return( 0);
  }
  else {
    return( lst->num);
  }
}

extern snet_expr_t *SNetEgetExpr( snet_expr_list_t *l, int num)
{
  if( l != NULL) {
    return( l->list[num]);  
  }
  else {
    return( NULL);
  }
}

/*
--------------------------------------------------------------------------------
*/

#define EXPR_OP( obj, num) obj->content.expr[num]

static bool isBoolean( snet_expr_t *expr) 
{
  bool result;

  result = 
      ( expr->type == CONSTB) ||
      ( expr->type == EQ) ||
      ( expr->type == NE) ||
      ( expr->type == GT) ||
      ( expr->type == GE) ||
      ( expr->type == LT) ||
      ( expr->type == LE) ||
      ( expr->type == AND)||
      ( expr->type == OR) ||
      ( expr->type == NOT);    

  return( result);
}


static int maxOp( int a, int b) 
{
  int res;

  res = a>b?a:b;

  return( res);
}

static int minOp( int a, int b) 
{
  int res;

  res = a<b?a:b;

  return( res);
}

static bool isEqualOp( snet_expr_t *expr, snet_record_t *rec) 
{
  
  bool result;

  if( isBoolean( expr->content.expr[0])) {
          result = 
            ( SNetEevaluateBool( EXPR_OP( expr, 0), rec) ==
              SNetEevaluateBool( EXPR_OP( expr, 1), rec));
        }
        else {
          result = 
            ( SNetEevaluateInt( EXPR_OP( expr, 0), rec) ==
              SNetEevaluateInt( EXPR_OP( expr, 1), rec));
        }
  return( result);
}

static bool isGreaterOp( snet_expr_t *expr, snet_record_t *rec) 
{
  bool result;

  result = 
    ( SNetEevaluateInt( EXPR_OP( expr, 0), rec) > 
      SNetEevaluateInt( EXPR_OP( expr, 1), rec));

  return( result);
}

static bool isLessOp( snet_expr_t *expr, snet_record_t *rec) 
{
  bool result;

  result = 
    ( SNetEevaluateInt( EXPR_OP( expr, 0), rec) <
      SNetEevaluateInt( EXPR_OP( expr, 1), rec));

  return( result);
}

extern bool SNetEevaluateBool( snet_expr_t *expr, snet_record_t *rec) 
{
  bool result;
  
  if( expr == NULL) {
    result = true;
  }
  else {
    switch( expr->type) {
      case CONSTB:
        result = *( expr->content.bval);
      break;
      case EQ:
        result = isEqualOp( expr, rec);
      break;
      case NE:
        result = !( isEqualOp( expr, rec));
      break;
      case GT:
        result = isGreaterOp( expr, rec);
      break;
      case GE:
        result = ( isGreaterOp( expr, rec) || isEqualOp( expr, rec));
      break;
      case LT:
        result = isLessOp( expr, rec);
      break;
      case LE:
        result = ( isLessOp( expr, rec) || isEqualOp( expr, rec));
      break;
      case AND:
        result = ( SNetEevaluateBool( EXPR_OP( expr, 0), rec) &&
                   SNetEevaluateBool( EXPR_OP( expr, 1), rec));
      break;
      case OR:
        result = ( SNetEevaluateBool( EXPR_OP( expr, 0), rec) ||
                   SNetEevaluateBool( EXPR_OP( expr, 1), rec));
      break;
      case NOT:
        result = !( SNetEevaluateBool( EXPR_OP( expr, 0), rec));
      break;
      default:
        printf("\n\n ** Runtime Error ** : Operation not handled "
               " in SNetEevaluateBool. [%d]\n\n", expr->type);
        exit( 1);
      break;
    }
  }


  return( result);
}

extern int SNetEevaluateInt( snet_expr_t *expr, snet_record_t *rec) 
{
  int result;
  
  if( expr == NULL) {
    result = true;
  }
  else {
    switch( expr->type) {
      case CONSTI:
        result = *( expr->content.ival);
      break;
      case TAG:
        result = SNetRecGetTag( rec, *(expr->content.ival));
      break;
      case BTAG:
        result = SNetRecGetBTag( rec, *(expr->content.ival));
      break;
      case ABS:
        result = abs( SNetEevaluateInt( EXPR_OP( expr, 0), rec));
      break;
      case MIN:
        result = minOp(
          SNetEevaluateInt( EXPR_OP( expr, 0), rec),
          SNetEevaluateInt( EXPR_OP( expr, 1), rec));
      break;
      case MAX:
        result = maxOp(
          SNetEevaluateInt( EXPR_OP( expr, 0), rec),
          SNetEevaluateInt( EXPR_OP( expr, 1), rec));
      break;
      case ADD:
        result =  
          SNetEevaluateInt( EXPR_OP( expr, 0), rec) +
          SNetEevaluateInt( EXPR_OP( expr, 1), rec);
      break;
      case MUL:
        result =  
          SNetEevaluateInt( EXPR_OP( expr, 0), rec) *
          SNetEevaluateInt( EXPR_OP( expr, 1), rec);
      break;
      case SUB:
        result =  
          SNetEevaluateInt( EXPR_OP( expr, 0), rec) -
          SNetEevaluateInt( EXPR_OP( expr, 1), rec);
      break;
      case DIV:
        result =  
          SNetEevaluateInt( EXPR_OP( expr, 0), rec) /
          SNetEevaluateInt( EXPR_OP( expr, 1), rec);
      break;
      case MOD:
        result =  
          SNetEevaluateInt( EXPR_OP( expr, 0), rec) %
          SNetEevaluateInt( EXPR_OP( expr, 1), rec);
      break;
      default:
        printf("\n\n ** Runtime Error ** : Operation not handled "
               " in SNetEevaluateInt. [%d]\n\n", expr->type);
        exit( 1);
      break;
    }
  }


  return( result);
}


