/*
 * This implements S-Net entities.
 */


#ifndef SNETENTITIES_H
#define SNETENTITIES_H


#include <buffer.h>
#include <handle.h>
#include <typeencode.h>
#include <constants.h>





extern snet_filter_instruction_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...);
extern void SNetDestroyFilterInstruction( snet_filter_instruction_t *instr);
extern snet_filter_instruction_set_t *SNetCreateFilterInstructionSet( int num, ...);	
extern void SNetDestroyFilterInstructionSet( snet_filter_instruction_set_t *set); 



extern snet_handle_t *SNetOutRaw( snet_handle_t *hnd, int variant_num, ...);

extern snet_handle_t *SNetOut( snet_handle_t *hnd, snet_record_t *rec);


extern snet_buffer_t *SNetBox( snet_buffer_t *inbuf, 
			       void (*boxfun)( snet_handle_t*), 
			       snet_typeencoding_t *outspec);


extern snet_buffer_t *SNetSerial( snet_buffer_t *inbuf, 
				  snet_buffer_t* (*box_a)( snet_buffer_t*), 
                                  snet_buffer_t* (*box_b)( snet_buffer_t*)); 

extern snet_buffer_t *SNetParallel( snet_buffer_t *inbuf,
				    snet_typeencoding_t *types, 
				    ...);



extern snet_buffer_t *SNetParallelDet( snet_buffer_t *inbuf, 
				       snet_typeencoding_t *types,
				       ...);


//extern void SNETstarDispatcher( snet_buffer_t *from, snet_buffer_t *to);
extern snet_buffer_t *SNetStar( snet_buffer_t *inbuf, 
				snet_typeencoding_t *type,
				snet_buffer_t* (*box_a)(snet_buffer_t*),  
                           	snet_buffer_t* (*box_b)(snet_buffer_t*));



extern snet_buffer_t *SNetStarIncarnate( snet_buffer_t *inbuf, 
					 snet_typeencoding_t *type,
					 snet_buffer_t* (*box_a)(snet_buffer_t*),  
                           		 snet_buffer_t* (*box_b)(snet_buffer_t*));

extern snet_buffer_t *SNetStarDet( snet_buffer_t *inbuf,
				   snet_typeencoding_t *type,
				   snet_buffer_t* (*box_a)(snet_buffer_t*),  
                           	   snet_buffer_t* (*box_b)(snet_buffer_t*));

extern snet_buffer_t *SNetStarDetIncarnate( snet_buffer_t *inbuf, 
					    snet_typeencoding_t *type,
					    snet_buffer_t* (*box_a)(snet_buffer_t*),  
                           		    snet_buffer_t* (*box_b)(snet_buffer_t*));


extern snet_buffer_t *SNetSync( snet_buffer_t *inbuf, 
		  	        snet_typeencoding_t *outtype, 
				snet_typeencoding_t *patterns);


extern snet_buffer_t *SNetSplit( snet_buffer_t *inbuf, 
				 snet_buffer_t* (*box_a)( snet_buffer_t*),
                            	 int ltag, 
				 int utag);

extern snet_buffer_t *SNetSplitDet( snet_buffer_t *inbuf, 
				    snet_buffer_t* (*box_a)( snet_buffer_t*),
                            	    int ltag, 
				    int utag);

extern snet_buffer_t *SNetFilter( snet_buffer_t *inbuf, 
				  snet_typeencoding_t *in_type,
				  snet_typeencoding_t *out_type, ...);

extern snet_buffer_t *SNetTranslate( snet_buffer_t *inbuf, 
				     snet_typeencoding_t *in_type,
				     snet_typeencoding_t *out_type, ...);


#endif
