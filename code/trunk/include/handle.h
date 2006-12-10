/*
 * this implements a handle and its accessor functions
 */

#ifndef HANDLE_H
#define HANDLE_H

#include <typeencode.h>
#include <record.h>
#include <buffer.h>




typedef enum {
	HND_box      = (1<<0),
	HND_parallel = (1<<1),
	HND_star     = (1<<2),
	HND_split    = (1<<3),
	HND_sync     = (1<<4),
	HND_filter   = (1<<5)
} snet_handledescriptor_t;

typedef enum {
	FLT_strip_tag,
	FLT_strip_field,
	FLT_add_tag,		
	FLT_set_tag,	
	FLT_rename_tag,
	FLT_rename_field,
	FLT_copy_field,
	FLT_use_tag,
	FLT_use_field
} snet_filter_opcode_t;


typedef struct handle snet_handle_t;

//typedef struct filter_instruction     snet_filter_instruction_t;
//typedef struct filter_instruction_set snet_filter_instruction_set_t;



typedef struct filter_instruction {
  snet_filter_opcode_t opcode;
  int *data;
} snet_filter_instruction_t;

typedef struct filter_instruction_set {
  int num;
  snet_filter_instruction_t **instructions;
} snet_filter_instruction_set_t;


// TODO: Add accepted descriptors to function comments
// TODO: Write the mentioned comments (or other way round?;)


/* Creates a handle, first parameter defines sort of handle.
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

extern void SNetHndSetInbuffer( snet_handle_t *hnd, snet_buffer_t *inbuf);
extern snet_buffer_t *SNetHndGetInbuffer( snet_handle_t *hndl);

extern snet_buffer_t *SNetHndGetOutbuffer( snet_handle_t *hndl);
extern snet_buffer_t *SNetHndGetOutbufferA( snet_handle_t *hndl);
extern snet_buffer_t *SNetHndGetOutbufferB( snet_handle_t *hndl);

extern void *SNetHndGetBoxfun( snet_handle_t *handle);
extern void *SNetHndGetBoxfunA( snet_handle_t *handle);
extern void *SNetHndGetBoxfunB( snet_handle_t *handle);

extern int SNetHndGetTagA( snet_handle_t *hndl);
extern int SNetHndGetTagB( snet_handle_t *hndl);

extern snet_typeencoding_t *SNetHndGetPatterns( snet_handle_t *hndl);

extern snet_typeencoding_t *SNetHndGetType( snet_handle_t *hndl);
extern snet_typeencoding_t *SNetHndGetInType( snet_handle_t *hnd);
extern snet_typeencoding_t *SNetHndGetOutType( snet_handle_t *hnd);
       
extern snet_filter_instruction_set_t **SNetHndGetFilterInstructions( snet_handle_t *hndl);

extern void SNetHndDestroy( snet_handle_t *hndl);

#endif
