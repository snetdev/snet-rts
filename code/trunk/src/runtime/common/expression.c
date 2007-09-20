/*
 * expression.c
 *
 *
 *
 */


#include <expression.h>

#include <memfun.h>


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
/*  
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
*/
  return( new);
}

static void DestroyExpr( snet_expr_t *expr) {

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
      break;
    case NOT:
    case ABS:
      SNetMemFree( expr->content.expr[0]);
      break;
    default:
      for( i=0; i<2; i++) {
        SNetMemFree( expr->content.expr[i]);
      }
  }
  SNetMemFree( expr);
}


extern snet_expr_t *SNetEconsti( int val) {
  snet_expr_t *new;
  int *value;

  value = SNetMemAlloc( sizeof( int));
  *value = val;
  new = CreateExpr( CONSTI);
  new->content.ival = value;
  
 return( new); 
}

extern snet_expr_t *SNetEconstb( bool val) {
  snet_expr_t *new;
  bool *value;

  value = SNetMemAlloc( sizeof( bool));
  *value = val;
  new = CreateExpr( CONSTB);
  new->content.bval = value;
  
 return( new); 
}

static snet_expr_t *CreateTagExpr( snet_expr_type_t t, int id) {
  snet_expr_t *new;
  int *value;

  value = SNetMemAlloc( sizeof( int));
  *value = id;
  new = CreateExpr( t);
  new->content.ival = value;
  
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

extern int SNetElistGetNum( snet_expr_list_t *lst) 
{
  return( lst->num);
}

extern snet_expr_t *SNetEgetExpr( snet_expr_list_t *l, int num)
{
  return( l->list[num]);
}

/*
--------------------------------------------------------------------------------
*/


static bool isBoolean( snet_expression_t *expr) 
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


extern bool SNetEevaluateBool( snet_expression_t *expr, snet_record_t *rec) 
{
  bool result;
  
  if( expr == NULL) {
    result = true;
  }
  else {
    switch( expr->type) {
      case CONSTI:
      break;
      case CONSTB:
        result = *( expr->content.bval);
      break;
      case TAG:
      break;
      case BTAG:
      break;
      case ABS:
      break;
      case MIN:
      break;
      case MAX:
      break;
      case ADD:
      break;
      case MUL:
      break;
      case SUB:
      break;
      case DIV:
      break;
      case EQ:
        if( isBoolean( expr->content.expr[0])) {
          result = 
            ( SNetEevaluateBool( expr->content.expr[0]) ==
              SNetEevaluateBool( expr->content.expr[1]));
        }
        else {
          result = 
            ( SNetEevaluateInt( expr->content.expr[0]) ==
              SNetEevaluateInt( expr->content.expr[1]));
        }
      break;
      case NE:
      break;
      case GT:
      break;
      case GE:
      break;
      case LT:
      break;
      case LE:
      break;
      case AND:
      break;
      case OR:
      break;
      case NOT:
      break;
      case COND:
      break;
    }
  }


  return( result);
}


