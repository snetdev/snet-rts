/*
 * This implements S-Net entities.
 * .
 * NOTE: all created threads are detached.
 */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <semaphore.h>

#include <memfun.h>
#include <record.h>
//#include <lbindings.h>
#include <bool.h>


#include <snetentities.h>


/* ************************************************************************* */
/* GLOBAL STRUCTURE                                                          */

#define INITIAL_INTERFACE_TABLE_SIZE    5

typedef struct {
 int id;
 void (*freefun)( void*);
 void* (*copyfun)( void*);
} snet_global_interface_functions_t;

typedef struct {
  int num;
  snet_global_interface_functions_t **interface;
} snet_global_info_structure_t;

snet_global_info_structure_t *snet_global = NULL;

static void SetFreeFun( snet_global_interface_functions_t *f,
                        void (*freefun)( void*))
{
  f->freefun = freefun;
}

static void SetCopyFun( snet_global_interface_functions_t *f,
                        void* (*copyfun)( void*))
{
  f->copyfun = copyfun;
}

static void *GetCopyFun( snet_global_interface_functions_t *f)
{
  return( f->copyfun);
}

static void *GetFreeFun( snet_global_interface_functions_t *f)
{
  return( f->freefun);
}

static void SetId( snet_global_interface_functions_t *f, int id)
{
  f->id = id;
}

static int GetId( snet_global_interface_functions_t *f)
{
  return( f->id);
}



static snet_global_interface_functions_t *GetInterface( int id) 
{
  int i;
  snet_global_interface_functions_t *result = NULL;

  for( i=0; i<snet_global->num; i++) {
    if( GetId( snet_global->interface[i]) == id) {
      result = snet_global->interface[i];
    }
  }
  if( result == NULL) {
    printf("\n\n ** Runtime Error ** : Interface ID (%d) not found!\n\n", id);
    exit( 1);
  }

  return( result);
}


static bool RuntimeInitialised()
{
  if( snet_global == NULL) {
    printf("\n\n ** Fatal Error ** : Runtime System not initialised!\n\n");
    exit( 1);
  }
  return( true);
}

extern bool SNetGlobalInitialise() 
{
  bool success = false;

  if( snet_global == NULL) {
    snet_global = SNetMemAlloc( sizeof( snet_global_info_structure_t));
    snet_global->interface =
      SNetMemAlloc( 
          INITIAL_INTERFACE_TABLE_SIZE * 
          sizeof( snet_global_interface_functions_t*));
    snet_global->num = 0;
    success = true;
  }
  else {
    printf("\n\n ** Fatal Error ** : Runtime System already initialised!\n\n");
    exit( 1);
  }
  return( success);
}


extern bool 
SNetGlobalRegisterInterface( int id, 
                             void (*freefun)( void*),
                             void* (*copyfun)( void*)) 
{
  int num; 
  snet_global_interface_functions_t *new_if;
  
  RuntimeInitialised();
  num = snet_global->num;
  if( num == ( INITIAL_INTERFACE_TABLE_SIZE -1)) {
    /* TODO replace this with proper code!!!! */
    printf("\n\n ** Runtime Error ** : Lookup table is full!\n\n");
    exit( 1);
  }
  else {
    new_if = SNetMemAlloc( sizeof( snet_global_interface_functions_t));  
    SetId( new_if, id);
    SetFreeFun( new_if, freefun);
    SetCopyFun( new_if, copyfun);   
    snet_global->interface[num] = new_if;
    snet_global->num += 1;
  }

  return( true);
}

extern void *SNetGetFreeFun( snet_record_t *rec) 
{
  return( GetFreeFun( GetInterface( SNetRecGetInterfaceId( rec))));
}

extern void *SNetGetCopyFun( snet_record_t *rec) 
{
  return( GetCopyFun( GetInterface( SNetRecGetInterfaceId( rec))));
}

/* END -- GLOBALS                                                            */
/* ************************************************************************* */


static void ThreadCreate( pthread_t *thread, 
                          pthread_attr_t *attrib, 
                          void *(*fun)(void*),
                          void *f_args) 
{
  int res;

  res = pthread_create( thread, attrib, fun, f_args);

  if( res != 0) {
    printf("\n\n  ** Fatal Error ** : Failed to create new "
           "thread [error %d].\n\n", res);
    fflush( stdout);
    exit( 1);
  }
}

static void ThreadDetach( pthread_t thread)
{
  int res;

  res = pthread_detach( thread);

  if( res != 0) {
    printf("\n\n  ** Fatal Error ** : Failed to detach "
           "thread [error %d].\n\n", res);
    fflush( stdout);
    exit( 1);
  }


}

/* --------------------------------------------------------
 * Syncro Cell: Flow Inheritance, uncomment desired variant
 * --------------------------------------------------------
 */
 
// inherit all fields and tags from matched records
//#define SYNC_FI_VARIANT_1

// inherit no tags/fields from previous matched records
// merge tags/fields from pattern to last arriving record.
#define SYNC_FI_VARIANT_2
/* --------------------------------------------------------
 */


#ifdef DEBUG
  #include <debug.h>
  extern debug_t *GLOBAL_OUTBUF_DBG;
  #define BUF_CREATE( PTR, SIZE)\
            PTR = SNetBufCreate( SIZE);\
            DBGregisterBuffer( GLOBAL_OUTBUF_DBG, PTR);

  #define BUF_SET_NAME( PTR, NAME)\
            DBGsetName( GLOBAL_OUTBUF_DBG, PTR, NAME);
#else
  #define BUF_CREATE( PTR, SIZE)\
            PTR = SNetBufCreate( SIZE);

  #define BUF_SET_NAME( PTR, NAME)
#endif

int warn_parallel = 0, warn_star = 0;



#define MC_ISMATCH( name) name->is_match
#define MC_COUNT( name) name->match_count
typedef struct {
  bool is_match;
  int match_count;
} match_count_t;




typedef struct {
  snet_buffer_t *from_buf;
  snet_buffer_t *to_buf;
  char *desc;
} dispatch_info_t;


typedef struct {
  snet_buffer_t *buf;
  int num;
} snet_blist_elem_t;

// used in SNetOut (depends on local variables!)
#define ENTRYSUM( RECNUM, TENCNUM)\
                        RECNUM( HNDgetRecord( hnd)) +\
                        TENCNUM( TENCgetVariant( HNDgetType( hnd), variant_num))

// used in SNetOut (depends on local variables!)
#define FILL_LOOP( RECNUM, RECNAME, TENCNUM, SET1, SET2 )\
  i = 0; cnt = 0;\
  while( cnt < RECNUM( HNDgetRecord( hnd))) {\
    int name;\
    name = RECNAME( HNDgetRecord( hnd))[i];\
    if( name != CONSUMED ) {\
      SET1 ;\
      cnt = cnt + 1;\
    }\
    i = i + 1;\
  }\
  for ( i=0; i < TENCNUM( TENCgetVariant( HNDgetType( hnd), variant_num)); i++) {\
    SET2 ;\
    cnt = cnt + 1;\
  }\

// used in SNetOut (depends on local variables!)
#define FILLVECTOR( RECNUM, RECNAME, NAMES, TENCNUM, TENCNAME) \
          FILL_LOOP( RECNUM, RECNAME, TENCNUM, \
            TENCsetVectorEntry( NAMES, cnt, RECNAME( HNDgetRecord( hnd))[i]),\
            TENCsetVectorEntry( NAMES, cnt, TENCNAME( TENCgetVariant( HNDgetType( hnd), variant_num))[i]))

// used in SNetOut (depends on local variables!)
#define FILLRECORD( RECNUM, RECNAME, RECSET, RECGET, TENCNUM, TENCNAME, TYPE) \
          FILL_LOOP( RECNUM, RECNAME, TENCNUM, \
            RECSET( outrec, name, RECGET( HNDgetRecord( hnd), name)),\
            RECSET( outrec, TENCNAME( TENCgetVariant( HNDgetType( hnd), variant_num))[i], va_arg( args, TYPE)))


// used in SNetParallelDet->CheckMatch (depends on local variables!)
#define FIND_NAME_LOOP( RECNUM, TENCNUM, RECNAMES, TENCNAMES)\
for( i=0; i<TENCNUM( venc); i++) {\
       if( !( ContainsName( TENCNAMES( venc)[i], RECNAMES( rec), RECNUM( rec)))) {\
        MC_ISMATCH( mc) = false;\
      }\
      else {\
        MC_COUNT( mc) += 1;\
      }\
    }



// used in SNetSync->IsMatch (depends on local variables!)
#define FIND_NAME_IN_PATTERN( TENCNUM, RECNUM, TENCNAMES, RECNAMES)\
  names = RECNAMES( rec);\
  for( i=0; i<TENCNUM( pat); i++) {\
    for( j=0; j<RECNUM( rec); j++) {\
      if( names[j] == TENCNAMES( pat)[i]) {\
        found_name = true;\
        break;\
      }\
    }\
    if ( !( found_name)) {\
      is_match = false;\
      break;\
    }\
    found_name = false;\
  }\
  SNetMemFree( names);


#define FILL_NAME_VECTOR( RECNAMES, RECNUM, TENCNAMES, TENCNUM, VECT_NAME)\
  last_entry = -1;\
  names = RECNAMES( rec);\
  for( i=0; i<RECNUM( rec); i++) {\
    TENCsetVectorEntry( VECT_NAME, i, names[i]);\
    last_entry = i;\
  }\
  SNetMemFree( names);\
  names = TENCNAMES( venc);\
  for( i=0; i<TENCNUM( venc); i++) {\
    TENCsetVectorEntry( VECT_NAME, ++last_entry, names[i]);\
  }


/* ------------------------------------------------------------------------- */
/*  SNetOut                                                                  */
/* ------------------------------------------------------------------------- */


extern snet_handle_t *SNetOut( snet_handle_t *hnd, snet_record_t *rec) {

  /* empty ... */
  printf("\n\n ** Fatal Error ** : Not implemented. [SNetOut]\n\n");
  exit( 1);

  return( hnd);
}

/* MERGE THE SNET_OUTS! */
extern snet_handle_t 
*SNetOutRawArray( snet_handle_t *hnd, 
                  int if_id,
                  int variant_num,
                  void **fields,
                  int *tags,
                  int *btags) 
{
  

  int i, *names;
  snet_record_t *out_rec, *old_rec;
  snet_variantencoding_t *venc;
  void* (*copyfun)(void*);

  venc = SNetTencGetVariant( SNetHndGetType( hnd), variant_num);
  
  if( variant_num > 0) {
    out_rec = SNetRecCreate( REC_data, SNetTencCopyVariantEncoding( venc));

    names = SNetTencGetFieldNames( venc);
    for( i=0; i<SNetTencGetNumFields( venc); i++) {
      SNetRecSetField( out_rec, names[i], fields[i]);
    }
    names = SNetTencGetTagNames( venc);
    for( i=0; i<SNetTencGetNumTags( venc); i++) {
      SNetRecSetTag( out_rec, names[i], tags[i]);
    }
    names = SNetTencGetBTagNames( venc);
    for( i=0; i<SNetTencGetNumBTags( venc); i++) {
      SNetRecSetBTag( out_rec, names[i], btags[i]);
    }
  }
  else {
    printf("\n\n ** Runtime Error ** : Variant <= 0. [SNetOut]\n\n");
    exit( 1);
  }
  
  SNetRecSetInterfaceId( out_rec, if_id);
  
  old_rec = SNetHndGetRecord( hnd);
  
  copyfun = GetCopyFun( GetInterface( SNetRecGetInterfaceId( old_rec)));

  names = SNetRecGetUnconsumedFieldNames( old_rec);
  for( i=0; i<SNetRecGetNumFields( old_rec); i++) {
    if( SNetRecAddField( out_rec, names[i])) {
      SNetRecSetField( out_rec, names[i], copyfun( SNetRecGetField( old_rec, names[i])));
    }
  }
  SNetMemFree( names);

  names = SNetRecGetUnconsumedTagNames( old_rec);
  for( i=0; i<SNetRecGetNumTags( old_rec); i++) {
    if( SNetRecAddTag( out_rec, names[i])) {
      SNetRecSetTag( out_rec, names[i], SNetRecGetTag( old_rec, names[i]));
    }
  }
  SNetMemFree( names);


  // output record
  SNetBufPut( SNetHndGetOutbuffer( hnd), out_rec);

  return( hnd);
}


extern snet_handle_t *SNetOutRaw( snet_handle_t *hnd, int variant_num, ...) {

  int i, *names;
  va_list args;
  snet_record_t *out_rec, *old_rec;
  snet_variantencoding_t *venc;


  // set values from box
  if( variant_num > 0) { 

   venc = SNetTencGetVariant( SNetHndGetType( hnd), variant_num);
   out_rec = SNetRecCreate( REC_data, SNetTencCopyVariantEncoding( venc));

   va_start( args, variant_num);
   names = SNetTencGetFieldNames( venc);
   for( i=0; i<SNetTencGetNumFields( venc); i++) {
     SNetRecSetField( out_rec, names[i], va_arg( args, void*));
   }
   names = SNetTencGetTagNames( venc);
   for( i=0; i<SNetTencGetNumTags( venc); i++) {
     SNetRecSetTag( out_rec, names[i], va_arg( args, int));
   }
   names = SNetTencGetBTagNames( venc);
   for( i=0; i<SNetTencGetNumBTags( venc); i++) {
     SNetRecSetBTag( out_rec, names[i], va_arg( args, int));
   }
   va_end( args);
  }
  else {
    printf("\n\n ** Runtime Error ** : Variant <= 0. [SNetOut]\n\n");
    exit( 1);
  }


  // flow inherit

  old_rec = SNetHndGetRecord( hnd);

  names = SNetRecGetUnconsumedFieldNames( old_rec);
  for( i=0; i<SNetRecGetNumFields( old_rec); i++) {
    if( SNetRecAddField( out_rec, names[i])) {
      SNetRecSetField( out_rec, names[i], SNetRecGetField( old_rec, names[i]));
    }
  }
  SNetMemFree( names);

  names = SNetRecGetUnconsumedTagNames( old_rec);
  for( i=0; i<SNetRecGetNumTags( old_rec); i++) {
    if( SNetRecAddTag( out_rec, names[i])) {
      SNetRecSetTag( out_rec, names[i], SNetRecGetTag( old_rec, names[i]));
    }
  }
  SNetMemFree( names);


  // output record
  SNetBufPut( SNetHndGetOutbuffer( hnd), out_rec);

  return( hnd);
}




/* ------------------------------------------------------------------------- */
/*  SNetBox                                                                  */
/* ------------------------------------------------------------------------- */

static void *BoxThread( void *hndl) {

  snet_handle_t *hnd;
  snet_record_t *rec;
  void (*boxfun)( snet_handle_t*);
  bool terminate = false;
  hnd = (snet_handle_t*) hndl;
  boxfun = SNetHndGetBoxfun( hnd);

  while( !( terminate)) {
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        SNetHndSetRecord( hnd, rec);
        (*boxfun)( hnd);
        SNetRecDestroy( rec);
        break;
      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        printf("\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_sort_end:
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;        
      case REC_terminate: 
        terminate = true;
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        SNetBufBlockUntilEmpty( SNetHndGetOutbuffer( hnd));
        SNetBufDestroy( SNetHndGetOutbuffer( hnd));
        SNetHndDestroy( hnd);
        break;
    }
  }
   return( NULL);
 
}

extern snet_buffer_t *SNetBox( snet_buffer_t *inbuf, void (*boxfun)( snet_handle_t*), 
                                   snet_typeencoding_t *outspec) {

  snet_buffer_t *outbuf;
  snet_handle_t *hndl;
  pthread_t *box_thread;
  

//  outbuf = SNetBufCreate( BUFFER_SIZE);
  BUF_CREATE( outbuf, BUFFER_SIZE);
  hndl = SNetHndCreate( HND_box, inbuf, outbuf, NULL, boxfun, outspec);

  box_thread = SNetMemAlloc( sizeof( pthread_t));
  ThreadCreate( box_thread, NULL, (void*)BoxThread, (void*)hndl );
  
  ThreadDetach( *box_thread);

  return( outbuf);
}
/* ------------------------------------------------------------------------- */
/*  SNetAlias                                                               */
/* ------------------------------------------------------------------------- */

extern snet_buffer_t *SNetAlias( snet_buffer_t *inbuf, 
                                 snet_buffer_t *(*net)(snet_buffer_t*)) {

  return( net( inbuf));
}


/* ------------------------------------------------------------------------- */
/*  SNetSerial                                                               */
/* ------------------------------------------------------------------------- */

extern snet_buffer_t *SNetSerial( snet_buffer_t *inbuf, 
                                  snet_buffer_t* (*box_a)( snet_buffer_t*),
                                  snet_buffer_t* (*box_b)( snet_buffer_t*)) {

  snet_buffer_t *internal_buf;
  snet_buffer_t *outbuf;

  internal_buf = (*box_a)( inbuf);
  outbuf = (*box_b)( internal_buf);

  return( outbuf);
}

/* ========================================================================= */
/* double linked list ------------------------------------------------------ */

typedef struct {
  void *elem;
  void *prev; 
  void *next;
} list_elem_t;

typedef struct {
  list_elem_t *head;
  list_elem_t *curr;
  list_elem_t *last;
  int elem_count;
} linked_list_t;

static linked_list_t *LLSTcreate( void *first_item) {
  linked_list_t *lst;
  list_elem_t *lst_elem;

  lst_elem = malloc( sizeof( list_elem_t));
  lst_elem->elem = first_item;
  lst_elem->prev = NULL;
  lst_elem->next = NULL;

  lst = malloc( sizeof( linked_list_t));
  lst->elem_count = 1;

  lst->head = lst_elem;
  lst->curr = lst_elem;
  lst->last = lst_elem;
  lst->head->prev = NULL;
  lst->head->prev = NULL;

  return( lst);
}

static void *LLSTget( linked_list_t *lst) {
  if( lst->elem_count > 0) {
    return( lst->curr->elem);
  }
  else {
    printf("\nWarning, returning NULL (list empty)\n\n");
    return( NULL);
  }
}

static void LLSTset( linked_list_t *lst, void *elem) {
  if( lst->elem_count > 0) {
    lst->curr->elem = elem;
  }
}

static void LLSTnext( linked_list_t *lst) {
  if( lst->elem_count > 0) {
    lst->curr = ( lst->curr->next == NULL) ? lst->head : lst->curr->next;
  }
}

/*
static void LLSTprev( linked_list_t *lst) {
  if( lst->elem_count > 0) {
    lst->curr = (lst->curr->prev == NULL) ? lst->last : lst->curr->prev;
  }
}
*/

static void LLSTadd( linked_list_t *lst, void *elem) {
  list_elem_t *new_elem;

  new_elem = malloc( sizeof( list_elem_t));
  new_elem->elem = elem;
  new_elem->next = NULL;
  new_elem->prev = lst->last;
  
  if( lst->elem_count == 0) {
    lst->head = new_elem;
    lst->curr = new_elem;
  }
  else {
    lst->last->next = new_elem;
  }
  lst->last = new_elem;

  lst->elem_count += 1;
}


static void LLSTremoveCurrent( linked_list_t *lst) {

  list_elem_t *tmp;

  if( lst->elem_count == 0) {
    printf("\n\n ** Fatal Error ** : Can't delete from empty list.\n\n");
    exit( 1);
  }

  // just one element left
  if( lst->head == lst->last) {
    SNetMemFree( lst->curr);
    lst->head = NULL;
    lst->curr = NULL;
    lst->last = NULL;
  }
  else {
    // deleting head
    if( lst->curr == lst->head) {
      ((list_elem_t*)lst->curr->next)->prev = NULL;
      tmp = lst->curr;
      lst->head = lst->curr->next;
      lst->curr = lst->curr->next;
      SNetMemFree( tmp);
    } 
    else {
      // deleting last element
      if( lst->curr == lst->last) {
        ((list_elem_t*)lst->curr->prev)->next = NULL;
        tmp = lst->curr;
        lst->last = lst->curr->prev;
        lst->curr = lst->curr->prev;
        SNetMemFree( tmp);
      }
      // delete ordinary element
      else {
      ((list_elem_t*)lst->curr->next)->prev = lst->curr->prev;  
      ((list_elem_t*)lst->curr->prev)->next = lst->curr->next;
      tmp = lst->curr;
      lst->curr = lst->curr->prev;
      SNetMemFree( tmp);
      }
    } 
  }
  lst->elem_count -= 1;
} 

static int LLSTgetCount( linked_list_t *lst) {
  if( lst != NULL) {
    return( lst->elem_count);
  }
  else {
    return( -1);
  }
}

static void LLSTdestroy( linked_list_t *lst) {

  if( LLSTgetCount( lst) > 0) {
    printf(" ** Info ** : Destroying non-empty list. [%d elements]\n\n", 
           LLSTgetCount( lst));
  }
  while( LLSTgetCount( lst) != 0) {
    LLSTremoveCurrent( lst);
    LLSTnext( lst);
  }
  SNetMemFree( lst);
}
/* ========================================================================= */





/* ============================================================= */
/* ============================================================= */



typedef struct {
  snet_buffer_t *buf;
  snet_record_t *current;
} snet_collect_elem_t;

static bool CompareSortRecords( snet_record_t *rec1, snet_record_t *rec2) {

  bool res;

  if( ( rec1 == NULL) || ( rec2 == NULL)) {  
    if( ( rec1 == NULL) && ( rec2 == NULL)) {
      res = true;
    }
    else {
      res = false;
    }
  }
  else {
    if( ( SNetRecGetLevel( rec1) == SNetRecGetLevel( rec2)) &&
        ( SNetRecGetNum( rec1) == SNetRecGetNum( rec2)))  {
     res = true;
   }
   else {
      res = false;
    }
  }

  return( res);
}


static void *DetCollector( void *info) {
  int i,j;
  dispatch_info_t *inf = (dispatch_info_t*)info;
  snet_buffer_t *outbuf = inf->to_buf;
  sem_t *sem;
  snet_record_t *rec;
  linked_list_t *lst = NULL;
  bool terminate = false;
  bool got_record;
  snet_collect_elem_t *tmp_elem, *new_elem, *elem;

  snet_record_t *current_sort_rec = NULL;
  
  int counter = 0; 
  int sort_end_counter = 0;

  bool processed_record;

  sem = SNetMemAlloc( sizeof( sem_t));
  sem_init( sem, 0, 0);

  SNetBufRegisterDispatcher( inf->from_buf, sem);
  
  elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
  elem->buf = inf->from_buf;
  elem->current = NULL;

  lst = LLSTcreate( elem);

  while( !( terminate)) {
    got_record = false;
    sem_wait( sem);
    do {
    processed_record = false;
    j = LLSTgetCount( lst);
    for( i=0; (i<j) && !(terminate); i++) {
//    while( !( got_record)) {
      tmp_elem = LLSTget( lst);
      rec = SNetBufShow( tmp_elem->buf);
      if( rec != NULL) { 
       
        got_record = true;
        
        switch( SNetRecGetDescriptor( rec)) {
          case REC_data:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              SNetBufPut( outbuf, rec);
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
                
            //rec = SNetBufGet( current_buf);
            //SNetBufPut( outbuf, rec);
            break;
  
          case REC_sync:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
             rec = SNetBufGet( tmp_elem->buf);
             tmp_elem->buf = SNetRecGetBuffer( rec);
             LLSTset( lst, tmp_elem);
             SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
             SNetRecDestroy( rec);
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
  
          case REC_collect:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
             rec = SNetBufGet( tmp_elem->buf);
             new_elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
             new_elem->buf = SNetRecGetBuffer( rec);
             if( current_sort_rec == NULL) {
               new_elem->current = NULL;
             }
             else {
               new_elem->current = SNetRecCopy( current_sort_rec);
             }
             LLSTadd( lst, new_elem);
             SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
             SNetRecDestroy( rec);
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }            
            break;
          case REC_sort_begin:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              tmp_elem->current = SNetRecCopy( rec);
              counter += 1;

              if( counter == LLSTgetCount( lst)) {
                counter = 0;
                current_sort_rec = SNetRecCopy( rec);

                if( SNetRecGetLevel( rec) != 0) {
                  SNetRecSetLevel( rec, SNetRecGetLevel( rec) - 1);
                  SNetBufPut( outbuf, rec);
                }
                else {
                   SNetRecDestroy( rec);
                }
              }
              else {
                 SNetRecDestroy( rec);
              }
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
          case REC_sort_end:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              sort_end_counter += 1;

              if( sort_end_counter == LLSTgetCount( lst)) {
                sort_end_counter = 0;

                if( SNetRecGetLevel( rec) != 0) {
                  SNetRecSetLevel( rec, SNetRecGetLevel( rec) - 1);
                  SNetBufPut( outbuf, rec);
                }
                else {
                  SNetRecDestroy( rec);
                }
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
          case REC_terminate:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              tmp_elem = LLSTget( lst);
              SNetBufDestroy( tmp_elem->buf);
              LLSTremoveCurrent( lst);
              SNetMemFree( tmp_elem);
              if( LLSTgetCount( lst) == 0) {
                terminate = true;
                SNetBufPut( outbuf, rec);
                SNetBufBlockUntilEmpty( outbuf);
                SNetBufDestroy( outbuf);
                LLSTdestroy( lst);
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record=true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }

            break;
        } // switch
      } // if
      else {
        LLSTnext( lst);
      }
//    } // while !got_record
    }
    } while( processed_record && !(terminate));
  } // while !terminate

  SNetMemFree( inf);
  return( NULL);
}

/* ============================================================= */
/* ============================================================= */



static void *Collector( void *info) {
  int i,j;
  dispatch_info_t *inf = (dispatch_info_t*)info;
  snet_buffer_t *outbuf = inf->to_buf;
  snet_buffer_t *current_buf;
  sem_t *sem;
  snet_record_t *rec;
  linked_list_t *lst = NULL;
  bool terminate = false;
  bool got_record;
  bool processed_record;
  snet_collect_elem_t *tmp_elem, *elem;

  snet_record_t *current_sort_rec = NULL;
  
  int counter = 0; 
  int sort_end_counter = 0;

  sem = SNetMemAlloc( sizeof( sem_t));
  sem_init( sem, 0, 0);

  SNetBufRegisterDispatcher( inf->from_buf, sem);
  
  elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
  elem->buf = inf->from_buf;
  elem->current = NULL;

  lst = LLSTcreate( elem);

  while( !( terminate)) {
    got_record = false;
    sem_wait( sem);
    do {
    got_record=false;
    processed_record = false;
    j = LLSTgetCount( lst);
    for( i=0; (i<j) && !(terminate); i++) {
//    while( !( got_record)) {
      current_buf = ((snet_collect_elem_t*) LLSTget( lst))->buf;
      rec = SNetBufShow( current_buf);
      if( rec != NULL) { 
       
        got_record = true;
        
        switch( SNetRecGetDescriptor( rec)) {
          case REC_data:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              SNetBufPut( outbuf, rec);
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
                
            //rec = SNetBufGet( current_buf);
            //SNetBufPut( outbuf, rec);
            break;
  
          case REC_sync:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
             rec = SNetBufGet( tmp_elem->buf);
             tmp_elem = LLSTget( lst);
             tmp_elem->buf = SNetRecGetBuffer( rec);
             LLSTset( lst, tmp_elem);
             SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
             SNetRecDestroy( rec);
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
  
          case REC_collect:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
             rec = SNetBufGet( tmp_elem->buf);
             tmp_elem = SNetMemAlloc( sizeof( snet_collect_elem_t));
             tmp_elem->buf = SNetRecGetBuffer( rec);
             if( current_sort_rec == NULL) {
               tmp_elem->current = NULL;
             }
             else {
               tmp_elem->current = SNetRecCopy( current_sort_rec);
             }
             LLSTadd( lst, tmp_elem);
             SNetBufRegisterDispatcher( SNetRecGetBuffer( rec), sem);
             SNetRecDestroy( rec);
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }            
            break;
          case REC_sort_begin:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              tmp_elem->current = SNetRecCopy( rec);
              counter += 1;

              if( counter == LLSTgetCount( lst)) {
                SNetRecDestroy( current_sort_rec);
                current_sort_rec = SNetRecCopy( rec);
                counter = 0;
                SNetBufPut( outbuf, rec);
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
          case REC_sort_end:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              sort_end_counter += 1;
              if( sort_end_counter == LLSTgetCount( lst)) {
                SNetBufPut( outbuf, rec);
                sort_end_counter = 0;
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }
            break;
          case REC_terminate:
            tmp_elem = LLSTget( lst);
            if( CompareSortRecords( tmp_elem->current, current_sort_rec)) {
              rec = SNetBufGet( tmp_elem->buf);
              tmp_elem = LLSTget( lst);
              SNetBufDestroy( tmp_elem->buf);
              LLSTremoveCurrent( lst);
              SNetMemFree( tmp_elem);
              if( LLSTgetCount( lst) == 0) {
                terminate = true;
                SNetBufPut( outbuf, rec);
                SNetBufBlockUntilEmpty( outbuf);
                SNetBufDestroy( outbuf);
                LLSTdestroy( lst);
              }
              else {
                SNetRecDestroy( rec);
              }
              processed_record = true;
            }
            else {
//              sem_post( sem);
              LLSTnext( lst);
            }

            break;
        } // switch
      } // if
      else {
        LLSTnext( lst);
      }
//    } // while !got_record
    } // for getCount
    } while( processed_record && !(terminate));
  } // while !terminate

  SNetMemFree( inf);
  return( NULL);
}


static snet_buffer_t *CollectorStartup( snet_buffer_t *initial_buffer, 
                                        bool is_det) {
                                          
  snet_buffer_t *outbuf;
  pthread_t *thread;
  dispatch_info_t *buffers;

  buffers = SNetMemAlloc( sizeof( dispatch_info_t));
  outbuf = SNetBufCreate( BUFFER_SIZE);

  buffers->from_buf = initial_buffer;
  buffers->to_buf = outbuf;


  thread = SNetMemAlloc( sizeof( pthread_t));
  
  if( is_det) {
    ThreadCreate( thread, NULL, DetCollector, (void*)buffers);
  }
  else {
    ThreadCreate( thread, NULL, Collector, (void*)buffers);
  }
  ThreadDetach( *thread);

  return( outbuf);
}

static snet_buffer_t *CreateCollector( snet_buffer_t *initial_buffer) {	

  return( CollectorStartup( initial_buffer, false));
}

static snet_buffer_t *CreateDetCollector( snet_buffer_t *initial_buffer) { 

  return( CollectorStartup( initial_buffer, true));
}

/* ------------------------------------------------------------------------- */
/*  SNetParallel non-deterministic                                           */
/* ------------------------------------------------------------------------- */



static bool ContainsName( int name, int *names, int num) {
  
  int i;
  bool found;

  found = false;

  for( i=0; i<num; i++) {
    if( names[i] == name) {
      found = true;
      break;
    }
  }

  return( found);
}

static match_count_t *CheckMatch( snet_record_t *rec, snet_variantencoding_t *venc, match_count_t *mc) {

  int i;

  MC_COUNT( mc) = 0;
  MC_ISMATCH( mc) = true;

  if( ( SNetRecGetNumFields( rec) < SNetTencGetNumFields( venc)) ||
      ( SNetRecGetNumTags( rec) < SNetTencGetNumTags( venc)) ||
      ( SNetRecGetNumBTags( rec) != SNetTencGetNumBTags( venc))) {
    MC_ISMATCH( mc) = false;
  }
  else { // is_match is set to value inside the macros
    FIND_NAME_LOOP( SNetRecGetNumFields, SNetTencGetNumFields,
        SNetRecGetFieldNames, SNetTencGetFieldNames);
    FIND_NAME_LOOP( SNetRecGetNumTags, SNetTencGetNumTags, 
        SNetRecGetTagNames, SNetTencGetTagNames);

    for( i=0; i<SNetRecGetNumBTags( rec); i++) {
       if( !( ContainsName( 
               SNetRecGetBTagNames( rec)[i], 
               SNetTencGetBTagNames( venc), 
               SNetTencGetNumBTags( venc)
               )
             )
           ) {
        MC_ISMATCH( mc) = false;
      }
      else {
       MC_COUNT( mc) += 1;
      }
    }

  }

  return( mc);
}

// Checks for "best match" and decides which buffer to dispatch to
// in case of a draw.
static int BestMatch( match_count_t **counter, int num) {

  int i;
  int res, max;
  
  res = -1;
  max = 0;
  for( i=0; i<num; i++) {
    if( MC_ISMATCH( counter[i])) {
      res = (MC_COUNT( counter[i]) > max) ? i : res;
    }
  }

  return( res);
}


static void PutToBuffers( snet_buffer_t **buffers, int num, 
                          int idx, snet_record_t *rec, int counter, bool det) {

  int i;

  for( i=0; i<num; i++) {
    if( det) {
      SNetBufPut( buffers[i], SNetRecCreate( REC_sort_begin, 0, counter));
    }

    if( i == idx) {
      SNetBufPut( buffers[i], rec);
    }

    if( det) {
      SNetBufPut( buffers[i], SNetRecCreate( REC_sort_end, 0, counter));
    }
  }
}

static void *ParallelBoxThread( void *hndl) {

  int i, num, buf_index;
  snet_handle_t *hnd = (snet_handle_t*) hndl;
  snet_record_t *rec;
  snet_buffer_t *go_buffer = NULL;
  match_count_t **matchcounter;
  snet_buffer_t **buffers;
  snet_typeencoding_t *types;

  bool is_det = SNetHndIsDet( hnd);
  
  bool terminate = false;
  int counter = 1;

  buffers = SNetHndGetOutbuffers( hnd);

  types = SNetHndGetType( hnd);
  num = SNetTencGetNumVariants( types);
  matchcounter = SNetMemAlloc( num * sizeof( match_count_t*));

  for( i=0; i<num; i++) {
    matchcounter[i] = SNetMemAlloc( sizeof( match_count_t));
  }

  while( !( terminate)) {
    
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        for( i=0; i<num; i++) {
          CheckMatch( rec, SNetTencGetVariant( types, i+1), matchcounter[i]);
        }
 
        buf_index = BestMatch( matchcounter, num);
        go_buffer = buffers[ buf_index];

        PutToBuffers( buffers, num, buf_index, rec, counter, is_det); 
        counter += 1;
      break;
      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        printf("\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
      case REC_sort_end:
        if( is_det) {
          SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        }
        for( i=0; i<(num-1); i++) {

          SNetBufPut( buffers[i], SNetRecCopy( rec));
        }
        SNetBufPut( buffers[num-1], rec);
        break;
      case REC_terminate:
        terminate = true;
        if( is_det) {
          for( i=0; i<num; i++) {
            SNetBufPut( buffers[i], 
                SNetRecCreate( REC_sort_begin, 0, counter));
          }
        }
        for( i=0; i<num; i++) {
          if( i == (num-1)) {
            SNetBufPut( buffers[i], rec);
          }
          else {
            SNetBufPut( buffers[i], SNetRecCopy( rec));
          }
        }
        for( i=0; i<num; i++) {
          SNetBufBlockUntilEmpty( buffers[i]);
          SNetBufDestroy( buffers[i]);
          SNetMemFree( matchcounter[i]);
        }
        SNetMemFree( buffers);
        SNetMemFree( matchcounter);
        SNetHndDestroy( hnd);
        break;
    }
  }  

  return( NULL);
}



static snet_buffer_t *SNetParallelStartup( snet_buffer_t *inbuf, 
                                           snet_typeencoding_t *types, 
                                           void **funs, bool is_det) {

  int i;
  int num;
  snet_handle_t *hnd;
  snet_buffer_t *outbuf;
  snet_buffer_t **transits;
  snet_buffer_t **outbufs;
  snet_buffer_t* (*fun)(snet_buffer_t*);
  pthread_t *box_thread;
  

  num = SNetTencGetNumVariants( types);
  outbufs = SNetMemAlloc( num * sizeof( snet_buffer_t*));
  transits = SNetMemAlloc( num * sizeof( snet_buffer_t*));


  transits[0] = SNetBufCreate( BUFFER_SIZE);
  fun = funs[0];
  outbufs[0] = (*fun)( transits[0]);

  if( is_det) {
    outbuf = CreateDetCollector( outbufs[0]);
  }
  else {
    outbuf = CreateCollector( outbufs[0]);
  }
 
  if( is_det) {
    SNetBufPut( outbufs[0], SNetRecCreate( REC_sort_begin, 0, 0));
  }
  for( i=1; i<num; i++) {

    transits[i] = SNetBufCreate( BUFFER_SIZE);

    fun = funs[i];
    outbufs[i] = (*fun)( transits[i]);
    SNetBufPut( outbufs[0], SNetRecCreate( REC_collect, outbufs[i]));
    if( is_det) {
      SNetBufPut( outbufs[i], SNetRecCreate( REC_sort_end, 0, 0));
    }
  }
  
  if( is_det) {
    SNetBufPut( outbufs[0], SNetRecCreate( REC_sort_end, 0, 0));
  }
  hnd = SNetHndCreate( HND_parallel, inbuf, transits, types, is_det);

  box_thread = SNetMemAlloc( sizeof( pthread_t));
  ThreadCreate( box_thread, NULL, ParallelBoxThread, (void*)hnd);
  ThreadDetach( *box_thread);

  SNetMemFree( funs);

  return( outbuf);
}


extern snet_buffer_t *SNetParallel( snet_buffer_t *inbuf,
                                    snet_typeencoding_t *types,
                                    ...) {


  va_list args;
  int i, num;
  void **funs;

  num = SNetTencGetNumVariants( types);

  funs = SNetMemAlloc( num * sizeof( void*)); 

  va_start( args, types);
  
  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, void*);
  }
  va_end( args);

  return( SNetParallelStartup( inbuf, types, funs, false));
}

extern snet_buffer_t *SNetParallelDet( snet_buffer_t *inbuf,
                                       snet_typeencoding_t *types,
                                       ...) {


  va_list args;
  int i, num;
  void **funs;

  num = SNetTencGetNumVariants( types);

  funs = SNetMemAlloc( num * sizeof( void*)); 

  va_start( args, types);
  
  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, void*);
  }
  va_end( args);

  return( SNetParallelStartup( inbuf, types, funs, true));
}

/* E------------------------------------------------------- */
/* E------------------------------------------------------- */
/* E------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/*  SNetStar                                                                 */
/* ------------------------------------------------------------------------- */


#define FIND_NAME_IN_RECORD( TENCNUM, TENCNAMES, RECNAMES, RECNUM)\
    for( j=0; j<TENCNUM( pat); j++) {\
      if( !( ContainsName( TENCNAMES( pat)[j],\
                           RECNAMES( rec),\
                           RECNUM( rec)))) {\
        is_match = false;\
        break;\
      }\
    }

static bool 
MatchesExitPattern( snet_record_t *rec, 
                    snet_typeencoding_t *patterns, 
                    snet_expr_list_t *guards) 
{

  int i,j;
  bool is_match;
  snet_variantencoding_t *pat;

  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {

    is_match = true;

    if( SNetEevaluateBool( SNetEgetExpr( guards, i), rec)) {
      pat = SNetTencGetVariant( patterns, i+1);

      FIND_NAME_IN_RECORD( SNetTencGetNumFields, SNetTencGetFieldNames, 
                           SNetRecGetFieldNames, SNetRecGetNumFields);
      if( is_match) {
        FIND_NAME_IN_RECORD( SNetTencGetNumTags, SNetTencGetTagNames, 
                             SNetRecGetTagNames, SNetRecGetNumTags);
        if( is_match) {
          FIND_NAME_IN_RECORD( SNetTencGetNumBTags, SNetTencGetBTagNames, 
                               SNetRecGetBTagNames, SNetRecGetNumBTags);
        }
      }
    }
    else {
      is_match = false;
    }

    if( is_match) {
      break;
    }
  }
  return( is_match);
}


static void *StarBoxThread( void *hndl) {

  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_buffer_t* (*box)( snet_buffer_t*);
  snet_buffer_t* (*self)( snet_buffer_t*);
  snet_buffer_t *real_outbuf, *our_outbuf, *starbuf=NULL;
  bool terminate = false;
  snet_typeencoding_t *exit_tags;
  snet_record_t *rec, *current_sort_rec = NULL;
  snet_expr_list_t *guards;

  real_outbuf = SNetHndGetOutbuffer( hnd);
  exit_tags = SNetHndGetType( hnd);
  box = SNetHndGetBoxfunA( hnd);
  self = SNetHndGetBoxfunB( hnd);
  guards = SNetHndGetGuardList( hnd);
        
  our_outbuf = SNetBufCreate( BUFFER_SIZE);

  while( !( terminate)) {
  
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        if( MatchesExitPattern( rec, exit_tags, guards)) {
          SNetBufPut( real_outbuf, rec);
        }
        else {
          if( starbuf == NULL) {
            // register new buffer with dispatcher,
            // starbuf is returned by self, which is SNetStarIncarnate
            starbuf = SNetSerial( our_outbuf, box, self);        
            SNetBufPut( real_outbuf, SNetRecCreate( REC_collect, starbuf));
/*            if( current_sort_rec != NULL) {
              printf("put\n");
              SNetBufPut( our_outbuf, SNetRecCreate( REC_sort_begin, 
                              SNetRecGetLevel( current_sort_rec), 
                              SNetRecGetNum( current_sort_rec)));
            }*/
         }
         SNetBufPut( our_outbuf, rec);
       }
       break;

      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
        printf("\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetRecDestroy( current_sort_rec);
        current_sort_rec = SNetRecCreate( REC_sort_begin, SNetRecGetLevel( rec),
                                          SNetRecGetNum( rec));
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCreate( REC_sort_begin,
                                    SNetRecGetLevel( rec),
                                    SNetRecGetNum( rec)));
        }
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_sort_end:
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCreate( REC_sort_end,
                                    SNetRecGetLevel( rec),
                                    SNetRecGetNum( rec)));
          
        }
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_terminate:
        terminate = true;
        SNetHndDestroy( hnd);
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCopy( rec));
        }
        SNetBufPut( real_outbuf, rec);
        SNetBufBlockUntilEmpty( our_outbuf);
        SNetBufDestroy( our_outbuf);
        //real_outbuf will be destroyed by the dispatcher
        break;
    }
  }

  return( NULL);
}

extern snet_buffer_t *SNetStar( snet_buffer_t *inbuf, 
                                snet_typeencoding_t *type,
                                snet_expr_list_t *guards,
                                snet_buffer_t* (*box_a)(snet_buffer_t*),
                                snet_buffer_t* (*box_b)(snet_buffer_t*)) {

    
  snet_buffer_t *star_outbuf, *dispatch_outbuf;
  snet_handle_t *hnd;
  pthread_t *box_thread;

  star_outbuf = SNetBufCreate( BUFFER_SIZE);
  
  hnd = SNetHndCreate( HND_star, inbuf, star_outbuf, box_a, box_b, type, guards, false);
  box_thread = SNetMemAlloc( sizeof( pthread_t));

  ThreadCreate( box_thread, NULL, StarBoxThread, hnd);
  ThreadDetach( *box_thread);

  dispatch_outbuf = CreateCollector( star_outbuf);
  
  return( dispatch_outbuf);
}


extern snet_buffer_t *SNetStarIncarnate( snet_buffer_t *inbuf,
                                         snet_typeencoding_t *type,
                                         snet_expr_list_t *guards,
                                         snet_buffer_t* (*box_a)(snet_buffer_t*),                                               snet_buffer_t* (*box_b)(snet_buffer_t*)) {

  snet_buffer_t *outbuf;
  snet_handle_t *hnd;
  pthread_t *box_thread;

  outbuf = SNetBufCreate( BUFFER_SIZE);


  hnd = SNetHndCreate( HND_star, inbuf, outbuf, box_a, box_b, type, guards, true);
  
  box_thread = SNetMemAlloc( sizeof( pthread_t));
  ThreadCreate( box_thread, NULL, StarBoxThread, hnd);
  ThreadDetach( *box_thread);

  return( outbuf);
}





/* ------------------------------------------------------------------------- */
/*  SNetStarDet                                                              */
/* ------------------------------------------------------------------------- */

static void *DetStarBoxThread( void *hndl) {

  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_buffer_t* (*box)( snet_buffer_t*);
  snet_buffer_t* (*self)( snet_buffer_t*);
  snet_buffer_t *real_outbuf, *our_outbuf, *starbuf=NULL;
  bool terminate = false;
  snet_typeencoding_t *exit_tags;
  snet_record_t *rec;
  snet_expr_list_t *guards;
 
  snet_record_t *sort_begin = NULL, *sort_end = NULL;
  int counter = 0;


  real_outbuf = SNetHndGetOutbuffer( hnd);
  exit_tags = SNetHndGetType( hnd);
  box = SNetHndGetBoxfunA( hnd);
  self = SNetHndGetBoxfunB( hnd);
  guards = SNetHndGetGuardList( hnd);
   
  our_outbuf = SNetBufCreate( BUFFER_SIZE);

  while( !( terminate)) {
  
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        if( !( SNetHndIsIncarnate( hnd))) { 
          SNetRecDestroy( sort_begin); SNetRecDestroy( sort_end);
          sort_begin = SNetRecCreate( REC_sort_begin, 0, counter);
          sort_end = SNetRecCreate( REC_sort_end, 0, counter);
        
          SNetBufPut( real_outbuf, SNetRecCopy( sort_begin));
          if( starbuf != NULL) {
            SNetBufPut( our_outbuf, SNetRecCopy( sort_begin));
          }
          
          if( MatchesExitPattern( rec, exit_tags, guards)) {
            SNetBufPut( real_outbuf, rec);
          }
          else {
            if( starbuf == NULL) {
              starbuf = SNetSerial( our_outbuf, box, self);        
              SNetBufPut( real_outbuf, SNetRecCreate( REC_collect, starbuf));
//              SNetBufPut( our_outbuf, sort_begin); 
            }
            SNetBufPut( our_outbuf, rec);
          }

          SNetBufPut( real_outbuf, SNetRecCopy( sort_end));
          if( starbuf != NULL) {
            SNetBufPut( our_outbuf, SNetRecCopy( sort_end));
          }
          counter += 1;
        }
        else { /* incarnation */
          if( MatchesExitPattern( rec, exit_tags, guards)) {
           SNetBufPut( real_outbuf, rec);
         }
         else {
           if( starbuf == NULL) {
             // register new buffer with dispatcher,
             // starbuf is returned by self, which is SNetStarIncarnate
             starbuf = SNetSerial( our_outbuf, box, self);        
             SNetBufPut( real_outbuf, SNetRecCreate( REC_collect, starbuf));
//             SNetBufPut( our_outbuf, sort_begin); /* sort_begin is set in "case REC_sort_xxx" */
          }
          SNetBufPut( our_outbuf, rec);
         }
        }
       break;

      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
        printf("\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_end:
      case REC_sort_begin:
        if( !( SNetHndIsIncarnate( hnd))) {
          SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        }
        SNetBufPut( SNetHndGetOutbuffer( hnd), SNetRecCopy( rec));
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCopy( rec));
        }
        else {
          if( SNetRecGetDescriptor( rec) == REC_sort_begin) {
            SNetRecDestroy( sort_begin);
            sort_begin = SNetRecCopy( rec);
          }
          else {
            SNetRecDestroy( sort_end);
            sort_end = SNetRecCopy( rec);
          }
        }
        SNetRecDestroy( rec);
        break;
      case REC_terminate:

        terminate = true;
        
        SNetHndDestroy( hnd);
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCreate( REC_sort_begin, 0, counter));
          SNetBufPut( our_outbuf, SNetRecCopy( rec));
        }
        SNetBufPut( real_outbuf, SNetRecCreate( REC_sort_begin, 0, counter));
        SNetBufPut( real_outbuf, rec);
        SNetBufBlockUntilEmpty( our_outbuf);
        SNetBufDestroy( our_outbuf);
        //real_outbuf will be destroyed by the dispatcher
        break;
    }
  }

  return( NULL);
}

extern snet_buffer_t *SNetStarDet( snet_buffer_t *inbuf,
                                   snet_typeencoding_t *type,
                                   snet_expr_list_t *guards,
                                   snet_buffer_t* (*box_a)(snet_buffer_t*),
                                   snet_buffer_t* (*box_b)(snet_buffer_t*)) {

    
  snet_buffer_t *star_outbuf, *dispatch_outbuf;
  snet_handle_t *hnd;
  pthread_t *box_thread;

  star_outbuf = SNetBufCreate( BUFFER_SIZE);
  
  hnd = SNetHndCreate( HND_star, inbuf, star_outbuf, box_a, box_b, type, guards, false);
  box_thread = SNetMemAlloc( sizeof( pthread_t));

  ThreadCreate( box_thread, NULL, DetStarBoxThread, hnd);
  ThreadDetach( *box_thread);

  dispatch_outbuf = CreateDetCollector( star_outbuf);
  
  return( dispatch_outbuf);
}


extern snet_buffer_t *SNetStarDetIncarnate( snet_buffer_t *inbuf,
                                            snet_typeencoding_t *type,  
                                            snet_expr_list_t *guards,
                                            snet_buffer_t* (*box_a)(snet_buffer_t*),
                                            snet_buffer_t* (*box_b)(snet_buffer_t*)) {
  snet_buffer_t *outbuf;
  snet_handle_t *hnd;
  pthread_t *box_thread;

  outbuf = SNetBufCreate( BUFFER_SIZE);

  hnd = SNetHndCreate( HND_star, inbuf, outbuf, box_a, box_b, type, guards, true);
  
  box_thread = SNetMemAlloc( sizeof( pthread_t));
  ThreadCreate( box_thread, NULL, DetStarBoxThread, hnd);
  ThreadDetach( *box_thread);

  return( outbuf);
}







/* ------------------------------------------------------------------------- */
/*  SNetSync                                                                 */
/* ------------------------------------------------------------------------- */


#ifdef SYNC_FI_VARIANT_2
// Destroys storage including all records
static snet_record_t *Merge( snet_record_t **storage, snet_typeencoding_t *patterns, 
						            snet_typeencoding_t *outtype, snet_record_t *rec) {

  int i,j, name, *names;
  snet_variantencoding_t *variant;
  
  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    variant = SNetTencGetVariant( patterns, i+1);
    names = SNetTencGetFieldNames( variant);
    for( j=0; j<SNetTencGetNumFields( variant); j++) {
      name = names[j];
      if( SNetRecAddField( rec, name)) {
        SNetRecSetField( rec, name, SNetRecTakeField( storage[i], name));  
      }
    }
    names = SNetTencGetTagNames( variant);
    for( j=0; j<SNetTencGetNumTags( variant); j++) {
      name = names[j];
      if( SNetRecAddTag( rec, name)) {
        SNetRecSetTag( rec, name, SNetRecGetTag( storage[i], name));  
      }
    }
  }

  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    if( storage[i] != rec) {
      SNetRecDestroy( storage[i]);
    }
  }
  
  return( rec);
}
#endif
#ifdef SYNC_FI_VARIANT_1
static snet_record_t *Merge( snet_record_t **storage, snet_typeencoding_t *patterns, 
                        snet_typeencoding_t *outtype) {

  int i,j;
  snet_record_t *outrec;
  snet_variantencoding_t *outvariant;
  snet_variantencoding_t *pat;
  outvariant = SNetTencCopyVariantEncoding( SNetTencGetVariant( outtype, 1));
  
  outrec = SNetRecCreate( REC_data, outvariant);

  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    pat = SNetTencGetVariant( patterns, i+1);
    for( j=0; j<SNetTencGetNumFields( pat); j++) {
      // set value in outrec of field (referenced by its new name) to the
      // according value of the original record (old name)
      if( storage[i] != NULL) {
        void *ptr;
        ptr = SNetRecTakeField( storage[i], SNetTencGetFieldNames( pat)[j]);
        SNetRecSetField( outrec, SNetTencGetFieldNames( pat)[j], ptr);
      }
      for( j=0; j<SNetTencGetNumTags( pat); j++) {
        int tag;
        tag = SNetRecTakeTag( storage[i], SNetTencGetTagNames( pat)[j]);
        SNetRecSetTag( outrec, SNetTencGetTagNames( pat)[j], tag);
      }
    }
  }
 
  
  // flow inherit all tags/fields that are present in storage
  // but not in pattern
  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    int *names, num;
    if( storage[i] != NULL) {

      names = SNetRecGetUnconsumedTagNames( storage[i]);
      num = SNetRecGetNumTags( storage[i]);
      for( j=0; j<num; j++) {
        int tag_value;
        if( SNetRecAddTag( outrec, names[j])) { // Don't overwrite existing Tags
          tag_value = SNetRecTakeTag( storage[i], names[j]);
          SNetRecSetTag( outrec, names[j], tag_value);
        }
      }
      SNetMemFree( names);

      names = SNetRecGetUnconsumedFieldNames( storage[i]);
      num = SNetRecGetNumFields( storage[i]);
      for( j=0; j<num; j++) {
        if( SNetRecAddField( outrec, names[j])) {
          SNetRecSetField( outrec, names[j], SNetRecTakeField( storage[i], names[j]));
        }
      }
      SNetMemFree( names);
    }
  }

  return( outrec);
}
#endif


static bool 
MatchPattern( snet_record_t *rec, 
              snet_variantencoding_t *pat,
              snet_expr_t *guard) {

  int i,j, *names;
  bool is_match, found_name;
  is_match = true;

  if( SNetEevaluateBool( guard, rec)) {
    found_name = false;
    FIND_NAME_IN_PATTERN( SNetTencGetNumTags, SNetRecGetNumTags, 
                          SNetTencGetTagNames, SNetRecGetUnconsumedTagNames);

    if( is_match) {
      FIND_NAME_IN_PATTERN( SNetTencGetNumFields, SNetRecGetNumFields, 
                            SNetTencGetFieldNames, SNetRecGetUnconsumedFieldNames);
    }
  }
  else {
    is_match = false;
  }

  return( is_match);
}



static void *SyncBoxThread( void *hndl) {

  int i;
  int match_cnt=0, new_matches=0;
  int num_patterns;
  bool was_match;
  bool terminate = false;
  snet_handle_t *hnd = (snet_handle_t*) hndl;
  snet_buffer_t *outbuf;
  snet_record_t **storage;
  snet_record_t *rec;
  snet_typeencoding_t *outtype;
  snet_typeencoding_t *patterns;
  snet_expr_list_t *guards;
  
  outbuf = SNetHndGetOutbuffer( hnd);
  outtype = SNetHndGetType( hnd);
  patterns = SNetHndGetPatterns( hnd);
  num_patterns = SNetTencGetNumVariants( patterns);
  guards = SNetHndGetGuardList( hnd);

  storage = SNetMemAlloc( num_patterns * sizeof( snet_record_t*));
  for( i=0; i<num_patterns; i++) {
    storage[i] = NULL;
  }
  
  while( !( terminate)) {
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        new_matches = 0;
      for( i=0; i<num_patterns; i++) {
        if( (storage[i] == NULL) && 
            (MatchPattern( rec, SNetTencGetVariant( patterns, i+1), SNetEgetExpr( guards, i)))) {
          storage[i] = rec;
          was_match = true;
          new_matches += 1;
        }
      }

      if( new_matches == 0) {
        SNetBufPut( outbuf, rec);
      } 
      else {
        match_cnt += new_matches;
       if( match_cnt == num_patterns) {
          #ifdef SYNC_FI_VARIANT_1
          SNetBufPut( outbuf, Merge( storage, patterns, outtype));
          #endif
          #ifdef SYNC_FI_VARIANT_2
          SNetBufPut( outbuf, Merge( storage, patterns, outtype, rec));
          #endif
          SNetBufPut( outbuf, SNetRecCreate( REC_sync, SNetHndGetInbuffer( hnd)));
          terminate = true;
       }
      }
      break;
    case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);

      break;
      case REC_collect:
        printf("\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_sort_end:
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
    case REC_terminate:
        printf("\n ** Runtime Warning: ** : Synchro cell received termination record."
               "\n                          Stored records are discarded! \n");
        terminate = true;
        SNetBufPut( outbuf, rec);
      break;
    }
  }

  SNetBufBlockUntilEmpty( outbuf);
  SNetBufDestroy( outbuf);
  SNetDestroyTypeEncoding( outtype);
  SNetDestroyTypeEncoding( patterns);
  SNetHndDestroy( hnd);
  SNetMemFree( storage);

  return( NULL);
}


extern snet_buffer_t *SNetSync( snet_buffer_t *inbuf, snet_typeencoding_t *outtype, 
                          snet_typeencoding_t *patterns, 
                          snet_expr_list_t *guards ) {

  snet_buffer_t *outbuf;
  snet_handle_t *hnd;
  pthread_t *box_thread;

//  outbuf = SNetBufCreate( BUFFER_SIZE);
  BUF_CREATE( outbuf, BUFFER_SIZE);
  hnd = SNetHndCreate( HND_sync, inbuf, outbuf, outtype, patterns, guards);

  box_thread = SNetMemAlloc( sizeof( pthread_t));

  ThreadCreate( box_thread, NULL, SyncBoxThread, (void*)hnd);
  ThreadDetach( *box_thread);

  return( outbuf);
}
                          




/* ------------------------------------------------------------------------- */
/*  SNetSplit                                                                */
/* ------------------------------------------------------------------------- */




snet_blist_elem_t *FindBufInList( linked_list_t *l, int num) {

  int i;
  snet_blist_elem_t *elem;
  for( i=0; i < LLSTgetCount( l); i++) {
    elem = (snet_blist_elem_t*)LLSTget( l);
    if( elem->num == num) {
      return( elem);
    } 
    else {
      LLSTnext( l);
    }
  }
  return( NULL);
}

static void *SplitBoxThread( void *hndl) {

  int i;
  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_buffer_t *initial;
  int ltag, ltag_val, utag, utag_val;
  snet_record_t *rec, *current_sort_rec;
  snet_buffer_t* (*boxfun)( snet_buffer_t*);
  bool terminate = false;
  linked_list_t *repos = NULL;
  snet_blist_elem_t *elem;


  initial = SNetHndGetOutbuffer( hnd);
  boxfun = SNetHndGetBoxfun( hnd);
  ltag = SNetHndGetTagA( hnd);
  utag = SNetHndGetTagB( hnd);

  
  while( !( terminate)) {
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        ltag_val = SNetRecGetTag( rec, ltag);
        utag_val = SNetRecGetTag( rec, utag);
        
        if( repos == NULL) {
          elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
          elem->num = ltag_val;
          elem->buf = SNetBufCreate( BUFFER_SIZE);
          SNetBufPut( initial, SNetRecCreate( REC_collect, boxfun( elem->buf)));
          //SNetBufBlockUntilEmpty( initial);
          repos = LLSTcreate( elem);
        }

        for( i = ltag_val; i <= utag_val; i++) {
          elem = FindBufInList( repos, i);
          if( elem == NULL) {
            elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
            elem->num = i;
            elem->buf = SNetBufCreate( BUFFER_SIZE);
            LLSTadd( repos, elem);
            SNetBufPut( initial, SNetRecCreate( REC_collect, boxfun( elem->buf)));
            //SNetBufBlockUntilEmpty( initial);
          } 
          if( i == utag_val) {
            SNetBufPut( elem->buf, rec); // last rec is not copied.
          }
          else {
            SNetBufPut( elem->buf, SNetRecCopy( rec)); // COPY
          }
        } 
        break;
      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);

        break;
      case REC_collect:
        printf("\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        current_sort_rec = rec;
        for( i=0; i<( LLSTgetCount( repos) - 1); i++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, 
              SNetRecCreate( REC_sort_begin, SNetRecGetLevel( rec), 
                             SNetRecGetNum( rec)));
          LLSTnext( repos);
        }
        elem = LLSTget( repos);
        SNetBufPut( elem->buf, rec);
        break;
      case REC_sort_end:
        current_sort_rec = rec;
        for( i=0; i<( LLSTgetCount( repos) - 1); i++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, 
              SNetRecCreate( REC_sort_end, SNetRecGetLevel( rec), 
                             SNetRecGetNum( rec)));
          LLSTnext( repos);
        }
        elem = LLSTget( repos);
        SNetBufPut( elem->buf, rec);      
        break;
      case REC_terminate:
        terminate = true;

        for( i=0; i<LLSTgetCount( repos); i++) {
          elem = LLSTget( repos);
          SNetBufPut( (snet_buffer_t*)elem->buf, SNetRecCopy( rec));
          LLSTnext( repos);
        }
        while( LLSTgetCount( repos) > 0) {
          LLSTremoveCurrent( repos);
          LLSTnext( repos);
        }
        LLSTdestroy( repos);

        SNetBufPut( initial, rec);
        break;
    }
  }

  return( NULL);
}



extern snet_buffer_t *SNetSplit( snet_buffer_t *inbuf, 
                                 snet_buffer_t* (*box_a)( snet_buffer_t*),
                                 int ltag, int utag) {

  snet_buffer_t *initial, *outbuf;
  snet_handle_t *hnd;
  pthread_t *box_thread;

  initial = SNetBufCreate( BUFFER_SIZE);
  hnd = SNetHndCreate( HND_split, inbuf, initial, box_a, ltag, utag);
 
  outbuf = CreateCollector( initial);

  box_thread = SNetMemAlloc( sizeof( pthread_t));
  ThreadCreate( box_thread, NULL, SplitBoxThread, (void*)hnd);
  ThreadDetach( *box_thread);

  return( outbuf);
}



/* B------------------------------------------------------- */
/* B Det Split    Playground ------------------------------ */
/* B------------------------------------------------------- */


static void *DetSplitBoxThread( void *hndl) {

  int i,j;
  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_buffer_t *initial, *tmp;
  int ltag, ltag_val, utag, utag_val;
  snet_record_t *rec;
  snet_buffer_t* (*boxfun)( snet_buffer_t*);
  bool terminate = false;
  linked_list_t *repos = NULL;
  snet_blist_elem_t *elem;

  int counter = 1;


  initial = SNetHndGetOutbuffer( hnd);
  boxfun = SNetHndGetBoxfun( hnd);
  ltag = SNetHndGetTagA( hnd);
  utag = SNetHndGetTagB( hnd);

  
  while( !( terminate)) {
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        ltag_val = SNetRecGetTag( rec, ltag);
        utag_val = SNetRecGetTag( rec, utag);
        
        if( repos == NULL) {
          elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
          elem->num = ltag_val;
          elem->buf = SNetBufCreate( BUFFER_SIZE);
          SNetBufPut( initial, SNetRecCreate( REC_sort_begin, 0, 0));
          
          tmp = boxfun( elem->buf);
          SNetBufPut( initial, SNetRecCreate( REC_collect, tmp));
//          SNetBufPut( tmp, SNetRecCreate( REC_sort_begin, 0, 0));
          SNetBufPut( tmp, SNetRecCreate( REC_sort_end, 0, 0));

          SNetBufPut( initial, SNetRecCreate( REC_sort_end, 0, 0));
          repos = LLSTcreate( elem);
        }


        SNetBufPut( initial, SNetRecCreate( REC_sort_begin, 0, counter)); 
        for( j=0; j<LLSTgetCount( repos); j++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, SNetRecCreate( REC_sort_begin, 0, counter)); 
          LLSTnext( repos);
        }

        for( i = ltag_val; i <= utag_val; i++) {
          elem = FindBufInList( repos, i);
          if( elem == NULL) {
            elem = SNetMemAlloc( sizeof( snet_blist_elem_t));
            elem->num = i;
            elem->buf = SNetBufCreate( BUFFER_SIZE);
            LLSTadd( repos, elem);
            SNetBufPut( initial, SNetRecCreate( REC_collect, boxfun( elem->buf)));
//            SNetBufPut( elem->buf, SNetRecCreate( REC_sort_begin, 0, counter));
          }
          if( i == utag_val) {
            SNetBufPut( elem->buf, rec); // last rec is not copied.
          }
          else {
            SNetBufPut( elem->buf, SNetRecCopy( rec)); // COPY
          }
        }

        for( j=0; j<LLSTgetCount( repos); j++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, SNetRecCreate( REC_sort_end, 0, counter)); 
          LLSTnext( repos);
        }
        SNetBufPut( initial, SNetRecCreate( REC_sort_end, 0, counter)); 
        counter += 1;

        break;
      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        printf("\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_sort_end:
        SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_terminate:
        terminate = true;

        for( i=0; i<LLSTgetCount( repos); i++) {
          elem = LLSTget( repos);
          SNetBufPut( elem->buf, SNetRecCreate( REC_sort_begin, 0, counter)); 
          SNetBufPut( (snet_buffer_t*)elem->buf, SNetRecCopy( rec));
          LLSTnext( repos);
        }
        while( LLSTgetCount( repos) > 0) {
          LLSTremoveCurrent( repos);
          LLSTnext( repos);
        }
        LLSTdestroy( repos);

        SNetBufPut( initial, SNetRecCreate( REC_sort_begin, 0, counter)); 
        SNetBufPut( initial, rec);
        break;
    }
  }

  return( NULL);
}




extern snet_buffer_t *SNetSplitDet( snet_buffer_t *inbuf, snet_buffer_t* (*box_a)( snet_buffer_t*),
                            int ltag, int utag) {

  snet_buffer_t *initial, *outbuf;
  snet_handle_t *hnd;
  pthread_t *box_thread;

  initial = SNetBufCreate( BUFFER_SIZE);
  hnd = SNetHndCreate( HND_split, inbuf, initial, box_a, ltag, utag);
 
  outbuf = CreateDetCollector( initial);

  box_thread = SNetMemAlloc( sizeof( pthread_t));
  ThreadCreate( box_thread, NULL, DetSplitBoxThread, (void*)hnd);
  ThreadDetach( *box_thread);

  return( outbuf);
}


/* ------------------------------------------------------------------------- */
/*  SNetFilter                                                               */
/* ------------------------------------------------------------------------- */


extern snet_filter_instruction_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...) {

  va_list args;
  snet_filter_instruction_t *instr;

  instr = SNetMemAlloc( sizeof( snet_filter_instruction_t));
  instr->opcode = opcode;
  instr->data = NULL;
  instr->expr = NULL;

  va_start( args, opcode);   

  switch( opcode) {
#ifdef FILTER_VERSION_2
    case create_record: // noop
    break;
    case snet_tag:
    case snet_btag:
      instr->data = SNetMemAlloc( sizeof( int));
      instr->data[0] = va_arg( args, int);
      instr->expr = va_arg( args, snet_expr_t*);
      break;
#else
    case FLT_add_tag:
    case FLT_strip_field:
    case FLT_strip_tag:
    case FLT_use_field:
    case FLT_use_tag:
      instr->data = SNetMemAlloc( sizeof( int));
      instr->data[0] = va_arg( args, int);
      break;
    case FLT_set_tag:
      instr->data = SNetMemAlloc( sizeof( int));
      //instr->expr = SNetMemAlloc( sizeof( snet_expr_t*));
      instr->data[0] = va_arg( args, int);
      instr->expr = va_arg( args, snet_expr_t*);
      break;
    case FLT_rename_tag:
    case FLT_rename_field:
    case FLT_copy_field:
#endif
#ifdef FILTER_VERSION_2
    case snet_field:
#endif

      instr->data = SNetMemAlloc( 2 * sizeof( int));
      instr->data[0] = va_arg( args, int);
      instr->data[1] = va_arg( args, int);
      break;
    default:
      printf("\n ** Fatal Error ** : Filter operation (%d) not implemented.\n\n", opcode);
      exit( 1);
  }
  
  va_end( args);

  
  return( instr);  
}


extern void SNetDestroyFilterInstruction( snet_filter_instruction_t *instr) {
  
  SNetMemFree( instr->data);
  //SNetMemFree( instr->expr);
  SNetMemFree( instr);
}

extern snet_filter_instruction_set_t *SNetCreateFilterInstructionSet( int num, ...) {

  va_list args;
  int i;
  snet_filter_instruction_set_t *set;

  set = SNetMemAlloc( sizeof( snet_filter_instruction_set_t));

  set->num = num;
  set->instructions = SNetMemAlloc( num * sizeof( snet_filter_instruction_t*));

  va_start( args, num);

  for( i=0; i<num; i++) {
    set->instructions[i] = va_arg( args, snet_filter_instruction_t*);
  }

  va_end( args);

  return( set);
}


extern int SNetFilterGetNumInstructions( snet_filter_instruction_set_t *set) 
{
  if( set == NULL) {
    return( 0);
  }
  else {
    return( set->num);
  }
}


extern snet_filter_instruction_set_list_t *SNetCreateFilterInstructionSetList( int num, ...) {
  va_list args;
  int i;
  snet_filter_instruction_set_list_t *lst;

  lst = SNetMemAlloc( sizeof( snet_filter_instruction_set_list_t));
  lst->num = num;
  lst->lst = SNetMemAlloc( num * sizeof( snet_filter_instruction_set_t*));
  
  va_start( args, num);
  for( i=0; i<num; i++) {
    (lst->lst)[i] = va_arg( args, snet_filter_instruction_set_t*);
  }
  va_end( args);

  return( lst);
}


extern void SNetDestroyFilterInstructionSet( snet_filter_instruction_set_t *set) {

  int i;

  for( i=0; i<set->num; i++) {
    SNetDestroyFilterInstruction( set->instructions[i]);
  }

  SNetMemFree( set->instructions);
  SNetMemFree( set);
}
#ifdef FILTER_VERSION_2

// pass at least one set!! -> lst must not be NULL!
static snet_typeencoding_list_t 
*FilterComputeTypes( int num, 
                     snet_filter_instruction_set_list_t **lst) 
{
  int i, j, k;
  snet_typeencoding_t **type_array;
  snet_filter_instruction_set_t **sets;
  snet_filter_instruction_set_t *current_set;
  snet_filter_instruction_t *current_instr;

  type_array = SNetMemAlloc( num * sizeof( snet_typeencoding_t*));

    for( i=0; i<num; i++) {
      
      sets = SNetFilterInstructionsGetSets( lst[i]);
      type_array[i] = SNetTencTypeEncode( 0);
      for( j=0; j<SNetFilterInstructionsGetNumSets( lst[i]); j++) {
        bool variant_created = false;
        current_set = sets[j];
        for( k=0; k<current_set->num; k++) {
          current_instr = current_set->instructions[k];
          switch( current_instr->opcode) {
            case snet_tag:
              if( variant_created) {
                SNetTencAddTag( 
                    SNetTencGetVariant( type_array[i], j+1), 
                    current_instr->data[0]);
              }
              else {
                 SNetTencAddVariant( type_array[i], 
                                      SNetTencVariantEncode( 
                                        SNetTencCreateEmptyVector( 0),
                                        SNetTencCreateEmptyVector( 0),
                                        SNetTencCreateEmptyVector( 0)));
                variant_created = true;               
              }
              break;
            case snet_btag:
              if( variant_created) {
                SNetTencAddBTag( 
                    SNetTencGetVariant( type_array[i], j+1), 
                    current_instr->data[0]);
              }
              else {
                 SNetTencAddVariant( type_array[i], 
                                      SNetTencVariantEncode( 
                                        SNetTencCreateEmptyVector( 0),
                                        SNetTencCreateEmptyVector( 0),
                                        SNetTencCreateEmptyVector( 0)));
                variant_created = true;               
              }                
              break;
            case snet_field:
              if( variant_created) {
                SNetTencAddField( 
                      SNetTencGetVariant( type_array[i], j+1), 
                      current_instr->data[0]);
              }
              else {
                 SNetTencAddVariant( type_array[i], 
                                      SNetTencVariantEncode( 
                                        SNetTencCreateEmptyVector( 0),
                                        SNetTencCreateEmptyVector( 0),
                                        SNetTencCreateEmptyVector( 0)));
                variant_created = true;               
              }              
              break;
            case create_record:
              if( !variant_created) {
                SNetTencAddVariant( type_array[i], 
                                      SNetTencVariantEncode( 
                                        SNetTencCreateEmptyVector( 0),
                                        SNetTencCreateEmptyVector( 0),
                                        SNetTencCreateEmptyVector( 0)));
                variant_created = true;
              }
              else {
                printf("\n\n ** Runtime Error ** : record_create applied to"
                       " non-empty type variant. [Filter]\n\n");
                exit( 1);
              }
              break;
            default:
              printf("\n\n ** Fatal Error ** : Unknown opcode"
                     " in filter instruction. [%d]\n\n", current_instr->opcode);
              exit( 1);
          }
        }
      }
    }
#ifdef DEBUG
  printf("\n<DEBUG_INF::Filter> ---------------- >");
  printf("\nNum types: %d\n", num);
  for( i=0; i<num; i++) {
    int num_vars;
    snet_typeencoding_t *t;
    snet_variantencoding_t *v;
    
    t = type_array[i];
    num_vars = SNetTencGetNumVariants( t);
    printf("Num variants in type %d: %d\n", i, num_vars);
    for( j=0; j<num_vars; j++) {
      printf("\nVariant %d:\n", j);
      v = SNetTencGetVariant( t, j+1);
      printf("\n  - %2d fields: ", SNetTencGetNumFields( v));
      for( k=0; k<SNetTencGetNumFields( v); k++) {
        printf(" %d ", SNetTencGetFieldNames( v)[k]);
      }
      printf("\n  - %2d tags  : ", SNetTencGetNumTags( v));
      for( k=0; k<SNetTencGetNumTags( v); k++) {
        printf(" %d ", SNetTencGetTagNames( v)[k]);
      }
      printf("\n  - %2d btags : ", SNetTencGetNumBTags( v));
      for( k=0; k<SNetTencGetNumBTags( v); k++) {
        printf(" %d ", SNetTencGetBTagNames( v)[k]);
      }
      printf("\n<----------------------------------- <\n\n");
    }
  }
#endif


  return( SNetTencCreateTypeEncodingListFromArray( num, type_array));
}

static int getNameFromInstr( snet_filter_instruction_t *instr) 
{
  return( instr->data[0]);
}

static int getFieldNameFromInstr( snet_filter_instruction_t *instr) 
{
  return( instr->data[1]);
}

static snet_expr_t *getExprFromInstr( snet_filter_instruction_t *instr) 
{
  return( instr->expr);
}
 
static snet_filter_instruction_t 
*FilterGetInstruction( snet_filter_instruction_set_t *set, int num) 
{
  if( set == NULL) {
    return( NULL);
  } 
  else {
    return( set->instructions[num]);
  }
}

static snet_filter_instruction_set_t
*FilterGetInstructionSet( snet_filter_instruction_set_list_t *l, int num) 
{
  if( l == NULL) {
    return( NULL);
  }
  else {
    return( l->lst[num]);
  }
}

static bool FilterInTypeHasName( int *names, int num, int name) 
{
  int i;
  bool result;

  result = false;

  for( i=0; i<num; i++) {
    if( names[i] == name) {
      result = true;
      break;
    }
  }

  return( result);
}

static bool FilterInTypeHasField( snet_typeencoding_t *t, int name) 
{
  snet_variantencoding_t *v;

  v = SNetTencGetVariant( t, 1);

  return( FilterInTypeHasName( 
            SNetTencGetFieldNames( v),
            SNetTencGetNumFields( v),
            name));
}

static bool FilterInTypeHasTag( snet_typeencoding_t *t, int name) 
{
  snet_variantencoding_t *v;

  v = SNetTencGetVariant( t, 1);

  return( FilterInTypeHasName( 
            SNetTencGetTagNames( v),
            SNetTencGetNumTags( v),
            name));
}

static snet_record_t 
*FilterInheritFromInrec( snet_typeencoding_t *in_type, 
                         snet_record_t *in_rec, 
                         snet_record_t *out_rec)
{
  int i;
  int *names;

  names = SNetRecGetFieldNames( in_rec);
  void* (*copyfun)(void*);
  copyfun = GetCopyFun( GetInterface( SNetRecGetInterfaceId( in_rec)));
  for( i=0; i<SNetRecGetNumFields( in_rec); i++) {
    if( !( FilterInTypeHasField( in_type, SNetRecGetFieldNames( in_rec)[i]))) {
      if( SNetRecAddField( out_rec, names[i])) {
        SNetRecSetField( out_rec, names[i], 
                         copyfun( SNetRecGetField( in_rec, names[i]))); 
      }
    }
  }

  names = SNetRecGetTagNames( in_rec);
  for( i=0; i<SNetRecGetNumTags( in_rec); i++) {
    if( !( FilterInTypeHasTag( in_type, SNetRecGetTagNames( in_rec)[i]))) {
      if( SNetRecAddTag( out_rec, names[i])) {
        SNetRecSetTag( out_rec, names[i], 
                         SNetRecGetTag( in_rec, names[i])); 
      }
    }
  }
  
  return( out_rec);
}

static void *FilterThread( void *hnd) 
{
  int i,j,k;
  bool done, terminate;
  snet_buffer_t *inbuf, *outbuf;
  snet_record_t *in_rec;
  snet_expr_list_t *guard_list;
  snet_typeencoding_t *out_type, *in_type;
  snet_typeencoding_list_t *type_list;
  snet_filter_instruction_t *current_instr;
  snet_filter_instruction_set_t *current_set; 
  snet_filter_instruction_set_list_t **instr_lst, *current_lst;

  done = false;
  terminate = false;
  
  inbuf = SNetHndGetInbuffer( hnd);
  outbuf = SNetHndGetOutbuffer( hnd);
  guard_list = SNetHndGetGuardList( hnd);
  in_type = SNetHndGetInType( hnd);
  type_list = SNetHndGetOutTypeList( hnd); 
  instr_lst = SNetHndGetFilterInstructionSetLists( hnd);

  while( !( terminate)) {
    in_rec = SNetBufGet( inbuf);
    done = false;

    switch( SNetRecGetDescriptor( in_rec)) {
      case REC_data:
          for( i=0; i<SNetElistGetNumExpressions( guard_list); i++) {
              if( ( SNetEevaluateBool( SNetEgetExpr( guard_list, i), in_rec)) 
                  && !( done)) { 
                snet_record_t *out_rec = NULL;
                done = true;
                out_type = SNetTencGetTypeEncoding( type_list, i);
                current_lst = instr_lst[i];
                for( j=0; j<SNetTencGetNumVariants( out_type); j++) {
                  out_rec = SNetRecCreate( REC_data, 
                                           SNetTencCopyVariantEncoding( 
                                           SNetTencGetVariant( out_type, j+1)));
                  SNetRecSetInterfaceId( out_rec, SNetRecGetInterfaceId( in_rec));
  
                  current_set = FilterGetInstructionSet( current_lst, j);
                  for( k=0; k<SNetFilterGetNumInstructions( current_set); k++) {
                    current_instr = FilterGetInstruction( current_set, k);
                    switch( current_instr->opcode) {
                      case snet_tag:
                        SNetRecSetTag( 
                            out_rec, 
                            getNameFromInstr( current_instr),
                            SNetEevaluateInt( 
                              getExprFromInstr( current_instr), in_rec));
                        break;
                      case snet_btag:
                        SNetRecSetBTag( 
                            out_rec, 
                            getNameFromInstr( current_instr),
                            SNetEevaluateInt( getExprFromInstr( current_instr), in_rec));
                        break;
                      case snet_field: {
                        void* (*copyfun)(void*);
                        copyfun = GetCopyFun( GetInterface( SNetRecGetInterfaceId( in_rec)));
                        SNetRecSetField( 
                            out_rec, 
                            getNameFromInstr( current_instr),
                            copyfun( SNetRecGetField( in_rec, getFieldNameFromInstr( current_instr))));
                        }
                        break;
                        case create_record: //noop
                        break;
                      default:
                        printf("\n\n ** Fatal Error ** : Unknown opcode in filter"
                               " instruction. [%d]\n\n", current_instr->opcode);
                        exit( 1);
                    } 
                  } // forall instructions of current_set

                  SNetBufPut( outbuf, 
                              FilterInheritFromInrec( in_type, in_rec, out_rec));
                } // forall variants of selected out_type
              SNetRecDestroy( in_rec);
              } // if guard is true
            } // forall guards
            if( !( done)) {
              printf("\n\n ** Runtime Error ** : All guards "
                     "evluated to FALSE. [Filter]\n\n");
              exit( 1);
            }
        break; // case rec_data
        case REC_sync:
          SNetHndSetInbuffer( hnd, SNetRecGetBuffer( in_rec));
          inbuf = SNetRecGetBuffer( in_rec);
          printf("\ninbuf\n");
          SNetRecDestroy( in_rec);
        break;
        case REC_collect:
          printf("\n:: DEBUG INFO ::  Unhandled control record, destroying it.\n\n");
          SNetRecDestroy( in_rec);
        break;
        case REC_sort_begin:
          SNetBufPut( SNetHndGetOutbuffer( hnd), in_rec);
        break;
        case REC_sort_end:
          SNetBufPut( SNetHndGetOutbuffer( hnd), in_rec);
        break;
        case REC_terminate:
          terminate = true;
          SNetBufPut( outbuf, in_rec);
          SNetBufBlockUntilEmpty( outbuf);
          SNetBufDestroy( outbuf);
          SNetHndDestroy( hnd);
        break;
    } // switch rec_descr
    
  } // while not terminate
    
  return( NULL);
}


extern snet_buffer_t 
*SNetFilter( snet_buffer_t *inbuf,
             snet_typeencoding_t *in_type,
             snet_expr_list_t *guards, ... ) 
{

  int i;
  int num_outtypes;
  snet_handle_t *hnd;
  snet_buffer_t *outbuf;
  snet_expr_list_t *guard_expr;
  snet_filter_instruction_set_list_t **instr_list;
  snet_typeencoding_list_t *out_types;
  pthread_t *filter_thread;
  va_list args;

  outbuf = SNetBufCreate( BUFFER_SIZE);
  guard_expr = guards;

  if( guard_expr == NULL) {
    guard_expr = SNetEcreateList( 1, SNetEconstb( true));
  }

  num_outtypes = SNetElistGetNumExpressions( guard_expr);

  instr_list = SNetMemAlloc( num_outtypes * sizeof( snet_filter_instruction_set_list_t*));
  
  va_start( args, guards);
  for( i=0; i<num_outtypes; i++) {
    instr_list[i] = va_arg( args, snet_filter_instruction_set_list_t*);
  }
  va_end( args);
  

  out_types = FilterComputeTypes( num_outtypes, instr_list);

  hnd = SNetHndCreate( HND_filter, inbuf, outbuf, in_type, out_types, guard_expr, instr_list);

  filter_thread = SNetMemAlloc( sizeof( pthread_t));
  ThreadCreate( filter_thread, NULL, FilterThread, (void*)hnd);
  ThreadDetach( *filter_thread);

  return( outbuf);
}
                       


extern snet_buffer_t 
*SNetTranslate( snet_buffer_t *inbuf,
                snet_typeencoding_t *in_type,
                snet_expr_list_t *guards, ... ) 
{

  int i;
  int num_outtypes;
  snet_handle_t *hnd;
  snet_buffer_t *outbuf;
  snet_expr_list_t *guard_expr;
  snet_filter_instruction_set_list_t **instr_list;
  snet_typeencoding_list_t *out_types;
  pthread_t *filter_thread;
  va_list args;

  outbuf = SNetBufCreate( BUFFER_SIZE);
  guard_expr = guards;

  if( guard_expr == NULL) {
    guard_expr = SNetEcreateList( 1, SNetEconstb( true));
  }

  num_outtypes = SNetElistGetNumExpressions( guard_expr);

  instr_list = SNetMemAlloc( num_outtypes * sizeof( snet_filter_instruction_set_list_t*));
  
  va_start( args, guards);
  for( i=0; i<num_outtypes; i++) {
    instr_list[i] = va_arg( args, snet_filter_instruction_set_list_t*);
  }
  va_end( args);
  

  out_types = FilterComputeTypes( num_outtypes, instr_list);

  hnd = SNetHndCreate( HND_filter, inbuf, outbuf, in_type, out_types, guard_expr, instr_list);

  filter_thread = SNetMemAlloc( sizeof( pthread_t));
  ThreadCreate( filter_thread, NULL, FilterThread, (void*)hnd);
  ThreadDetach( *filter_thread);

  return( outbuf);
}
#else
static void *FilterThread( void *hndl) {

  int i,j;
  snet_variantencoding_t *names;
  snet_handle_t *hnd = (snet_handle_t*) hndl;
  snet_buffer_t *outbuf;
  snet_typeencoding_t *out_type;
  snet_filter_instruction_set_t **instructions, *set;
  snet_filter_instruction_t *instr;
  snet_variantencoding_t *variant;
  snet_record_t *out_rec, *in_rec;
  bool terminate = false;

  outbuf = SNetHndGetOutbuffer( hnd);
//  in_variant = SNetTencGetVariant( SNetHndGetInType( hnd), 1);
  out_type =  SNetHndGetOutType( hnd);

  instructions = SNetHndGetFilterInstructions( hnd);

  while( !( terminate)) {
    in_rec = SNetBufGet( SNetHndGetInbuffer( hnd));
    switch( SNetRecGetDescriptor( in_rec)) {
      case REC_data:
        for( i=0; i<SNetTencGetNumVariants( out_type); i++) {
        names = SNetTencCopyVariantEncoding( SNetRecGetVariantEncoding( in_rec));
        variant = SNetTencCopyVariantEncoding( SNetTencGetVariant( out_type, i+1));
        out_rec = SNetRecCreate( REC_data, variant);
        SNetRecSetInterfaceId( out_rec, SNetRecGetInterfaceId( in_rec));
        set = instructions[i];
        // this runs for each filter instruction 
        for( j=0; j<set->num; j++) {
          instr = set->instructions[j];
          switch( instr->opcode) {
            case FLT_strip_tag:
              SNetTencRemoveTag( names, instr->data[0]);
              break;
            case FLT_strip_field:
//              SNetFreeField( 
//                  SNetRecGetField( in_rec, instr->data[0]),
//                  SNetRecGetLanguage( in_rec));
              SNetTencRemoveField( names, instr->data[0]);
              break;
            case FLT_add_tag:
              SNetRecSetTag( out_rec, instr->data[0], 0); 
              break;
            case FLT_set_tag:
              // expressions are ignored for now!
              SNetRecSetTag( out_rec, instr->data[0], 0);
              break;
            case FLT_rename_tag:
              SNetRecSetTag( out_rec, instr->data[1], 
                  SNetRecGetTag( in_rec, instr->data[0]));
              SNetTencRemoveTag( names, instr->data[0]);
              break;
            case FLT_rename_field:
              SNetRecSetField( out_rec, instr->data[1], 
                  SNetRecGetField( in_rec, instr->data[0]));
              SNetTencRemoveField( names, instr->data[0]);
              break;
            case FLT_copy_field: {
              void* (*copyfun)(void*);
              copyfun = GetCopyFun( GetInterface( SNetRecGetInterfaceId( in_rec)));
              SNetRecSetField( out_rec, instr->data[0],
                copyfun( SNetRecGetField( in_rec, instr->data[0])));
              }
              break;
            case FLT_use_tag:
              SNetRecSetTag( out_rec, instr->data[0],
                  SNetRecGetTag( in_rec, instr->data[0]));
              break;
            case FLT_use_field:
              SNetRecSetField( out_rec, instr->data[0],
                  SNetRecGetField( in_rec, instr->data[0]));
              break;
          }
        }
        // flow inherit, remove everthing that is already present in the 
        // outrecord. Renamed fields/tags are removed above.
        // TODO: remove everything that is present in in_type but not in out_type
        for( j=0; j<SNetTencGetNumFields( variant); j++) {
//          SNetFreeField( 
//              SNetRecGetField( in_rec, SNetTencGetFieldNames( variant)[j]),
//              SNetRecGetLanguage( in_rec));
          SNetTencRemoveField( names, SNetTencGetFieldNames( variant)[j]);
        }
        for( j=0; j<SNetTencGetNumTags( variant); j++) {
          SNetTencRemoveTag( names, SNetTencGetTagNames( variant)[j]);
        }
  
       // add everything that is left.
        void* (*copyfun)(void*);
        copyfun = GetCopyFun( GetInterface( SNetRecGetInterfaceId( in_rec)));
        for( j=0; j<SNetTencGetNumFields( names); j++) {
         if( SNetRecAddField( out_rec, SNetTencGetFieldNames( names)[j])) {
            SNetRecSetField( out_rec, 
                SNetTencGetFieldNames( names)[j],
                copyfun( SNetRecGetField( in_rec, SNetTencGetFieldNames( names)[j])));
         }
       }  
      for( j=0; j<SNetTencGetNumTags( names); j++) {
         if( SNetRecAddTag( out_rec, SNetTencGetTagNames( names)[j])) {
            SNetRecSetTag( out_rec, 
                SNetTencGetTagNames( names)[j],
                SNetRecGetTag( in_rec, SNetTencGetTagNames( names)[j]));
         }
      }  

      SNetTencDestroyVariantEncoding( names);  
      SNetBufPut( outbuf, out_rec);
      }
      SNetRecDestroy( in_rec);
      break;
    case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( in_rec));
        SNetRecDestroy( in_rec);
      break;
      case REC_collect:
        printf("\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( in_rec);
        break;
      case REC_sort_begin:
        SNetBufPut( SNetHndGetOutbuffer( hnd), in_rec);
        break;
      case REC_sort_end:
        SNetBufPut( SNetHndGetOutbuffer( hnd), in_rec);
        break;
    case REC_terminate:
      terminate = true;
      SNetBufPut( outbuf, in_rec);
      SNetBufBlockUntilEmpty( outbuf);
      SNetBufDestroy( outbuf);
      SNetHndDestroy( hnd);
      break;
    }
  }  

  return( NULL);
}



static snet_buffer_t *CreateFilter( snet_buffer_t *inbuf, 
                                    snet_typeencoding_t *in_type,
                                    snet_typeencoding_t *out_type,
                                    snet_filter_instruction_set_t **set,
                                    bool is_translator) {

  snet_buffer_t *outbuf;
  snet_handle_t *hnd;
  pthread_t *box_thread;

  outbuf = SNetBufCreate( BUFFER_SIZE);
  hnd = SNetHndCreate( HND_filter, inbuf, outbuf, in_type, out_type, set);

  box_thread = SNetMemAlloc( sizeof( pthread_t));

  if( is_translator) { // for the time being, a translator is just a filter
    ThreadCreate( box_thread, NULL, FilterThread, (void*)hnd);
  } 
  else {
    ThreadCreate( box_thread, NULL, FilterThread, (void*)hnd);
  }
  
  ThreadDetach( *box_thread);

  return( outbuf);


}


extern snet_buffer_t *SNetFilter( snet_buffer_t *inbuf, 
                                  snet_typeencoding_t *in_type,   
                                  snet_typeencoding_list_t *out_types, 
                                  snet_expr_list_t *guards,  ...) {
  int i;
  va_list args;

  snet_filter_instruction_set_list_t *lst;
  snet_filter_instruction_set_t **set;
  snet_typeencoding_t *out_type;

  // *
  if( SNetTencGetNumTypes( out_types) > 1) {
    printf("\n\n ** Error ** : Requested feature not implemented yet (filter)\n\n");
    exit( 1);
  }

  out_type = SNetTencGetTypeEncoding( out_types, 0);

//  set = SNetMemAlloc( SNetTencGetNumVariants( out_type) * sizeof( snet_filter_instruction_set_t*));
  va_start( args, guards);

  for( i=0; i < SNetTencGetNumTypes( out_types) ; i++) {
    lst = va_arg( args, snet_filter_instruction_set_list_t*);
    set = SNetFilterInstructionsGetSets( lst); // !!!!!
  }
  va_end( args);

  return( CreateFilter( inbuf, in_type, out_type, set, false));
}


extern snet_buffer_t *SNetTranslate( snet_buffer_t *inbuf, 
                                     snet_typeencoding_t *in_type,   
                                     snet_typeencoding_t *out_type, ...) {
  int i;
  va_list args;

  snet_filter_instruction_set_t **set;

  set = SNetMemAlloc( SNetTencGetNumVariants( out_type) * sizeof( snet_filter_instruction_set_t*));

  va_start( args, out_type);
  for( i=0; i<SNetTencGetNumVariants( out_type); i++) {
    set[i] = va_arg( args, snet_filter_instruction_set_t*);
  }
  va_end( args);

  return( CreateFilter( inbuf, in_type, out_type, set, true));
}
#endif
