/*
 * Implements a record with OO style accessor functions.
 * NOTE that, once a tag/field is taken (RECtakeXxx), it cannot be
 * reused. No tag/field can be replaced by another one once it was set. 
 */

#ifndef RECORD_H
#define RECORD_H


typedef struct record snet_record_t;
typedef union record_types snet_record_types_t;

#include "typeencode.h"
#include "bool.h"
#include "constants.h"
#include "stream.h"

#ifdef DISTRIBUTED_SNET
#include <mpi.h>
#include "id.h"
#endif /* DISTRIBUTED_SNET */

/* 
 * data structure for the record
 */


typedef enum {
  REC_data,
  REC_sync,
  REC_collect,
  REC_sort_end,
  REC_terminate,
  REC_trigger_initialiser
} snet_record_descr_t;

typedef enum {
  MODE_textual,
  MODE_binary,
} snet_record_mode_t;


/* returns true if the record matches the pattern. */
extern bool SNetRecPatternMatches(snet_variantencoding_t *pat, struct record *rec);

/*
 * Allocates memory for a record and initialises
 * the datastructe. 
 * Allocates memory for pointer to fields, tags and
 * binding tags.
 * RETURNS: pointer to the record
 */

extern snet_record_t *SNetRecCreate( snet_record_descr_t descr, ...);

/*
 * CREC_sync: snet_buffer_t *inbuf
 *
 */ 

extern snet_record_descr_t SNetRecGetDescriptor( snet_record_t *rec);

extern stream_t *SNetRecGetStream( snet_record_t *rec);


/*
 * Creates copy of given record.
 * RETURN: pointer to copy
 */
extern snet_record_t *SNetRecCopy( snet_record_t *rec);

/*
 * Copies field 'old_name' from record 'from' to 
 * record 'to' with name 'new_name'.
 *
 * NOTICE: This should be used instead of "get-copy-set"!
 */
extern void SNetRecCopyFieldToRec( snet_record_t *from, int old_name, 
				   snet_record_t *to, int new_name);

extern void SNetRecRenameTag( snet_record_t *rec, int name, int new_name);
extern void SNetRecRenameBTag( snet_record_t *rec, int name, int new_name);
extern void SNetRecRenameField( snet_record_t *rec, int name, int new_name);


/*
 * Deletes the record. Frees memory.
 * RETURNS: nothing
 */

extern void SNetRecDestroy( snet_record_t *r);



/*
 * Sets Tag "id" to value "val". 
 * RETURNS: nothing.
 */

extern void SNetRecSetTag( snet_record_t *r, int id, int val);



/*
 * Gets value of tag "id". The tag is not removed from
 * the record.
 * RETURNS: value of tag
 */

extern int SNetRecGetTag( snet_record_t *r, int id);




/* 
 * Gets value of tag "id". The tag is removed from the record,
 * where "removed" means: the value of the tag ist set to CONSUMED
 * RETURNS: value of tag
 */

extern int SNetRecTakeTag( snet_record_t *r, int id);





/*
 * Returns number of (unconsumed!) Tags in the record.
 * RETURNS: number of tags
 */

extern int SNetRecGetNumTags( snet_record_t *r);


/*
 * Returns names of tags in the record, _including_ consumed tags
 * RETURNS pointer to the array of names in the type encoding of the record
 */

extern int *SNetRecGetTagNames( snet_record_t *r);



/*
 * Returns names of unconsumed tags.
 * A new array only containing the unconsumed names is build.
 * Please remember to SNetMemFree() the array after usage.
 * RETURNS pointer to newly allocated array
 */

extern int *SNetRecGetUnconsumedTagNames( snet_record_t *rec);




/*
 * Sets binding tag (see SNetRecSetTag)
 * RETURNS: nothing
 */

extern void SNetRecSetBTag( snet_record_t *r, int id, int val);


int SNetRecReadTag( snet_record_t *r, int id);


/* 
 *  Gets binding Tag (see SNetRecGetTag)
 * RETURNS: value of binding tag
 */

extern int SNetRecGetBTag( snet_record_t *r, int id);




/* 
 * Gets and removes binding tag. (see SNetRecTakeTag)
 * RETURNS: value of binding tag
 */
 
extern int SNetRecTakeBTag( snet_record_t *r, int id);
 

/*
 * Returns number of (unconsumed!) BTags in the record.
 * RETURNS: number of binding tags
 */

extern int SNetRecGetNumBTags( snet_record_t *r);



/*
 * Returns names of binding tags in the record, _including_ consumed btags 
 * RETURNS pointer to the array of names in the type encoding of the record
 */

extern int *SNetRecGetBTagNames( snet_record_t *r);




/*
 * Returns names of unconsumed binding tags.
 * A new array only containing the unconsumed names is build.
 * Please remember to SNetMemFree() the array after usage.
 * RETURNS pointer to newly allocated array
 */

extern int *SNetRecGetUnconsumedBTagNames( snet_record_t *rec);




/* 
 * Sets value of field (see SNetRecSetTag)
 * The value of a field is a void pointer to
 * the fields data.
 * RETURNS: nothing
 */

extern void SNetRecSetField( snet_record_t *r, int id, void* ptr);




/*
 * Gets value of field (see SNetRecGetTag)
 * The returned value is a void pointer.
 * RETURNS: pointer to field data
 */

extern void *SNetRecGetField( snet_record_t *r, int id);




/*
 * Gets value of field.
 * The returned value is a void pointer.
 * The field is removed from the record,
 * where "removed" means, the pointer is 
 * set to NULL.
 * RETURNS: pointer to field data
 */

extern void *SNetRecTakeField( snet_record_t *r, int id);

extern void SNetRecMoveFieldToRec( snet_record_t *from, int old_name ,
				   snet_record_t *to, int new_name);

void *SNetRecReadField( snet_record_t *r, int id);


/*
 * Returns number of (unconsumed!) fields in the record.
 * RETURNS: number of fields
 */

extern int SNetRecGetNumFields( snet_record_t *r);



/*
 * Returns names of fields in the record, _including_ consumed fields 
 * RETURNS pointer to the array of names in the type encoding of the record
 */

extern int *SNetRecGetFieldNames( snet_record_t *r);



/*
 * Returns names of unconsumed fields.
 * A new array only containing the unconsumed names is build.
 * Please remember to SNetMemFree() the array after usage.
 * RETURNS pointer to newly allocated array
 */

extern int *SNetRecGetUnconsumedFieldNames( snet_record_t *rec);




// adds tag. copies array of ints to a new 
// array, which has its size increased by one.
extern bool SNetRecAddTag( snet_record_t *rec, int name);
extern bool SNetRecAddBTag( snet_record_t *rec, int name);
extern bool SNetRecAddField( snet_record_t *rec, int name);

extern bool SNetRecHasTag( snet_record_t *rec, int name);
extern bool SNetRecHasBTag( snet_record_t *rec, int name);
extern bool SNetRecHasField( snet_record_t *rec, int name);


extern void SNetRecRemoveTag( snet_record_t *rec, int name);
extern void SNetRecRemoveBTag( snet_record_t *rec, int name);
extern void SNetRecRemoveField( snet_record_t *rec, int name);


extern void SNetRecSetNum( snet_record_t *rec, int value);
extern void SNetRecSetLevel( snet_record_t *rec, int value);
extern int SNetRecGetNum( snet_record_t *rec);
extern int SNetRecGetLevel( snet_record_t *rec);


//extern snet_lang_descr_t SNetRecGetLanguage( snet_record_t *rec);
//extern void SNetRecSetLanguage( snet_record_t *rec, snet_lang_descr_t lang);

extern snet_variantencoding_t *SNetRecGetVariantEncoding( snet_record_t *rec);

extern int SNetRecGetInterfaceId( snet_record_t *rec);
extern snet_record_t *SNetRecSetInterfaceId( snet_record_t *rec, int id);

extern snet_record_mode_t SNetRecGetDataMode( snet_record_t *rec);
extern snet_record_t *SNetRecSetDataMode( snet_record_t *rec, snet_record_mode_t mode);


#ifdef DISTRIBUTED_SNET
extern int SNetRecPack(snet_record_t *rec, MPI_Comm comm, int *pos, void *buf, int buf_size);
extern snet_record_t *SNetRecUnpack(MPI_Comm comm, int *pos, void *buf, int buf_size);
#endif

#endif

