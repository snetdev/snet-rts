/*
 * Implements type/variant/field encodings.
 */


#ifndef TYPEENCODE_H
#define TYPEENCODE_H

#include <bool.h>
typedef struct vector snet_vector_t;

typedef struct typeencode snet_typeencoding_t;
typedef struct typeencoding_list snet_typeencoding_list_t;

typedef struct variantencode snet_variantencoding_t;

typedef struct patternencoding snet_patternencoding_t;

typedef struct patternset snet_patternset_t;

struct record;
/* ****************************************************************** */



/* --- Vector Functions --- */

/* returns true if the record matches the pattern. */
extern bool SNetTencPatternMatches(snet_variantencoding_t *pat, struct record *rec);
/*
 * Creates a vector with "num" entries. 
 * The entries must be given as parameter (exactly "num" of them).
 * The entries must be int values, representing the names of
 * of fields/tags/binding tags.
 * RETURNS: pointer to the vector
 */

extern snet_vector_t *SNetTencCreateVector( int num, ...);




/*
 * Creates a vector for "num" elements.
 * All entries are uninitialised.
 * RETURNS: pointer to the vector
 */
                 
extern snet_vector_t *SNetTencCreateEmptyVector( int num);



/*
 * Sets entry "num" of the vector to "val".
 * RETURNS: nothing
 */

extern void SNetTencSetVectorEntry( snet_vector_t *vect, int num, int val);




/*
 * Returns the number of entries of a given vector.
 * RETURNS: number of entries.
 */

extern int SNetTencGetNumVectorEntries( snet_vector_t *vec);




extern void SNetTencRemoveUnsetEntries( snet_vector_t *vec);



/* 
 * Creates a copy of the given vector	 
 * RETURNS: pointer to  vector
 */

extern snet_vector_t *SNetTencCopyVector( snet_vector_t *vec);



extern void SNetTencDestroyVector( snet_vector_t *vec);


/* --- Variant Encoding Functions --- */


/* 
 * Creates a variant encoding. The parameters are vectors
 * of 1.) field names, 2.) tag names and 3.) binding tag names, 
 * exactly in this order. 
 * RETURNS: pointer to the variant encoding.
 */

extern snet_variantencoding_t *SNetTencVariantEncode( snet_vector_t *field_v, snet_vector_t *tag_v, snet_vector_t *btag_v);




/*
 * Creates copy of given variant encoding.
 * RETURNS: pointer to copy
 */

extern snet_variantencoding_t *SNetTencCopyVariantEncoding( snet_variantencoding_t *v_enc);




/*
 * Return true if the name is contained in the fields, false otherwise
 */
extern bool SNetTencContainsFieldName(snet_variantencoding_t *v_enc, int name);

/*
 * Return true if the name is contained in the tags, false otherwise
 */
extern bool SNetTencContainsTagName(snet_variantencoding_t *v_enc, int name);

/*
 * Return true if the name is contained in the binding tags, false otherwise
 */
extern bool SNetTencContainsBTagName(snet_variantencoding_t *v_enc, int name);

/* 
 * Returns the number of fields of a given variant encoding.
 * RETURNS: number of fields.
 */
 	 
extern int SNetTencGetNumFields( snet_variantencoding_t *v_enc);





/* 
 * Returns the names of all fields of the variant encoding.
 * RETURNS: int array containing names.
 */

extern int *SNetTencGetFieldNames( snet_variantencoding_t *v_enc); 





/* 
 * Returns the number of tags of a given variant encoding.
 * RETURNS: number of tags.
 */
 
extern int SNetTencGetNumTags( snet_variantencoding_t *v_enc);





/* 
 * Returns the names of all tags of the variant encoding.
 * RETURNS: int array containing names.
 */

extern int *SNetTencGetTagNames( snet_variantencoding_t *v_enc); 





/* 
 * Returns the number of binding tags of a given variant encoding.
 * RETURNS: number of binding tags.
 */
 
extern int SNetTencGetNumBTags( snet_variantencoding_t *v_enc);





/* 
 * Returns the names of all binding tags of the variant encoding.
 * RETURNS: int array containing names.
 */

extern int *SNetTencGetBTagNames( snet_variantencoding_t *v_enc); 





/*
 * Renames a Tag with id "name" to "newname". Meant to be
 * used with "TakeTag" to set name to e.g. "CONSUMED".
 * PLEASE NOTE: renameTag does _not_ remove any names, it just
 * renames them. 
 * RETURNS: nothing
 */

extern void SNetTencRenameTag( snet_variantencoding_t *v_enc, int name, int newName);






/*
 * Renames a bindingTag with id "name" to "newname". Meant to be
 * used with "TakeBTag" to set name to e.g. "CONSUMED".
 * RETURNS: nothing
 */

extern void SNetTencRenameBTag( snet_variantencoding_t *v_enc, int name, int newName);






/*
 * Renames a Field with id "name" to "newname". Meant to be
 * used with "TakeField" to set name to e.g. "CONSUMED".
 * RETURNS: nothing
 */

extern void SNetTencRenameField( snet_variantencoding_t *v_enc, int name, int newName);


extern bool SNetTencAddTag( snet_variantencoding_t *venc, int name);
extern bool SNetTencAddBTag( snet_variantencoding_t *venc, int name);
extern bool SNetTencAddField( snet_variantencoding_t *venc, int name);


/* 
 * Deletes the variant encoding and frees the memory
 * RETURNS: nothing
 */

extern void SNetTencDestroyVariantEncoding( snet_variantencoding_t *v_enc);



extern void SNetTencRemoveTag( snet_variantencoding_t *v_enc, int name);
extern void SNetTencRemoveBTag( snet_variantencoding_t *v_enc, int name);
extern void SNetTencRemoveField( snet_variantencoding_t *v_enc, int name);


/* --- Type Encoding Functions --- */


/*
 * Creates a type encoding of "num" variant encodings.
 * The parameters to this function must be pointers to
 * variant encodings.
 * RETURNS: pointer to type encoding
 */

extern snet_typeencoding_t 
*SNetTencTypeEncode( int num, ...);

extern snet_typeencoding_list_t 
*SNetTencCreateTypeEncodingList( int num, ...);

extern snet_typeencoding_list_t 
*SNetTencCreateTypeEncodingListFromArray( int num, snet_typeencoding_t **t);

extern int 
SNetTencGetNumTypes( snet_typeencoding_list_t *lst);

extern snet_typeencoding_t 
*SNetTencGetTypeEncoding( snet_typeencoding_list_t *lst, int num); 

extern snet_typeencoding_t 
*SNetTencAddVariant( snet_typeencoding_t *t,
                     snet_variantencoding_t *v);


/*
 * RETURNS: number of variants
 */

extern int SNetTencGetNumVariants( snet_typeencoding_t *type);





/*
 * RETURNS: variant of given num
 */

extern snet_variantencoding_t *SNetTencGetVariant( snet_typeencoding_t *type, int num);




/*
 * Deletes the type encoding and frees the memory.
 * RETURNS: nothing.
 */ 

extern void SNetDestroyTypeEncoding( snet_typeencoding_t *t_enc);




/* --- Pattern Encoding Functions --- */


/*
 * Create a pattern encoding. 
 * The pattern encoding is used to store one pattern for a synchro cell.
 * Since this must provide a renaming facility, 2*(num_tags + num_fields)
 * parameter have to be passed to the function. Every tag/field is listed 
 * twice: with its original name and with the name to which it will be 
 * renamed. If no renaming is necessary, the same name appears twice.
 * Example: Create encoding that contains Tag T_a and field F_a, T_a is
 * renamed to T_new whereas F_a ist not renamed.
 * SNetTencPatternEncode( 1, 1, T_a, T_new, F_a, F_a);
 * RETURNS: pointer to pattern encoding
 */

extern snet_patternencoding_t *SNetTencPatternEncode( int num_tags, int num_fields, ...);




extern int SNetTencGetNumPatternTags( snet_patternencoding_t *pat);
extern int *SNetTencGetPatternTagNames( snet_patternencoding_t *pat);
extern int *SNetTencGetPatternNewTagNames( snet_patternencoding_t *pat);
extern int SNetTencGetNumPatternFields( snet_patternencoding_t *pat);
extern int *SNetTencGetPatternFieldNames( snet_patternencoding_t *pat);
extern int *SNetTencGetPatternNewFieldNames( snet_patternencoding_t *pat);




extern void SNetTencDestroyPatternEncoding( snet_patternencoding_t *pat);




/*
 * Create a set of pattern encodings.
 * This set of encodings is meant to be used in a synchro cell.
 * It takes num patternencoding_ts as parameters and creates the set.
 * RETURNS: pointer to the set.
 */

extern snet_patternset_t *SNetTencCreatePatternSet( int num, ...);

extern int SNetTencGetNumPatterns( snet_patternset_t *patterns);

extern snet_patternencoding_t *SNetTencGetPatternEncoding( snet_patternset_t *patset, int num);


extern void SNetTencDestroyPatternSet( snet_patternset_t *patset);

#endif

