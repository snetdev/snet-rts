/* 
 * $Id: handle.h 2399 2009-05-19 15:59:39Z fpz $
 */

#ifndef _HANDLE_P_H_
#define _HANDLE_P_H_

//#include <string.h>

#include "handle.h"
#include "filter.h"
#include "typeencode.h"
#include "record.h"
#include "expression.h"

struct stream;

typedef enum {
	HND_box      = (1<<0),
	HND_parallel = (1<<1),
	HND_star     = (1<<2),
	HND_split    = (1<<3),
	HND_sync     = (1<<4),
	HND_filter   = (1<<5)
} snet_handledescriptor_t;

//typedef struct handle snet_handle_t;


// TODO: Add accepted descriptors to function comments
// TODO: Write the mentioned comments (or the other way round?;)


/* Creates a handle, first parameter defines type of handle.
 *  HND_box:			inbuf, outbuf, record, boxfun, outtype
 *	HND_parallel: inbuf, outbuf_a, outbuf_b, outtype
 *  HND_star: inbuf, outbuf, boxfun_a, boxfun_self(=boxfun_b), outtype   
 *  HND_sync: inbuf, outbuf, patterns
 *  HND_split: inbuf, outbuf, boxfun, tagA(=ltag), tagB(=utag)
 *  HND_filter: inbuf, outbuf, filter_instructions
 */
extern snet_handle_t *SNetHndCreate( snet_handledescriptor_t descr, ...);

extern snet_record_t *SNetHndGetRecord( snet_handle_t *hndl);

extern void SNetHndSetRecord( snet_handle_t *hndl, snet_record_t *rec);

extern void SNetHndSetInput( snet_handle_t *hnd, struct stream *inbuf);
extern struct stream *SNetHndGetInput( snet_handle_t *hndl);

extern struct stream *SNetHndGetOutput( snet_handle_t *hndl);
extern struct stream **SNetHndGetOutputs( snet_handle_t *hndl);

extern void *SNetHndGetBoxfun( snet_handle_t *handle);
extern void *SNetHndGetBoxfunA( snet_handle_t *handle);
extern void *SNetHndGetBoxfunB( snet_handle_t *handle);

extern int SNetHndGetTagA( snet_handle_t *hndl);
extern int SNetHndGetTagB( snet_handle_t *hndl);

extern snet_typeencoding_t *SNetHndGetPatterns( snet_handle_t *hndl);

extern snet_typeencoding_t *SNetHndGetType( snet_handle_t *hndl);
extern snet_typeencoding_t *SNetHndGetInType( snet_handle_t *hnd);
extern snet_typeencoding_list_t *SNetHndGetTypeList( snet_handle_t *hnd);

extern snet_filter_instruction_set_t **SNetHndGetFilterInstructions( snet_handle_t *hndl);

extern snet_filter_instruction_set_list_t
**SNetHndGetFilterInstructionSetLists( snet_handle_t *hnd);	

extern snet_typeencoding_list_t
*SNetHndGetOutTypeList( snet_handle_t *hnd);


extern snet_expr_list_t 
*SNetHndGetGuardList( snet_handle_t *hnd);


extern bool SNetHndIsIncarnate( snet_handle_t *hnd);

extern void SNetHndDestroy( snet_handle_t *hndl);
extern snet_handle_t *SNetHndCopy( snet_handle_t *hnd);

extern bool SNetHndIsDet( snet_handle_t *hnd);

#ifdef DISTRIBUTED_SNET
extern bool SNetHndIsSplitByLocation( snet_handle_t *hnd);
#endif /* DISTRIBUTED_SNET */

snet_box_sign_t *SNetHndGetBoxSign( snet_handle_t *hnd);

#endif
