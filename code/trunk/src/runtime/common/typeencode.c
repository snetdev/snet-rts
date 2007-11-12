/*
 * Implements type/variant/field encodings.
 */

// TODO: remove union from vector (not needed anymore)


#include <stdio.h>
#include <stdlib.h>
#include <memfun.h>

#include <typeencode.h>
#include <stdarg.h>
#include <constants.h>





#define TENC_CREATE_VEC( VEC, NUM, TYPE, NAME) \
          va_list args;\
          int i;\
          VEC = SNetMemAlloc( sizeof( snet_vector_t));\
          VEC->num = NUM;\
          if( NUM == 0) {\
             VEC->fields.NAME = NULL;\
          }\
          else {\
            VEC->fields.NAME = SNetMemAlloc( ( NUM) * sizeof( TYPE));\
            va_start(args, NUM);\
            for( i=0; i < NUM; i++) {\
              VEC->fields.NAME[i] = va_arg(args, TYPE);\
            }\
            va_end( args);\
          }

          


#define REMOVE_FROM_TENC( NAMES, TENCNUM, TENCNAMES)\
  int i, skip;\
  snet_vector_t *new;\
  bool is_present = false;\
  for( i=0; i<TENCNUM( venc); i++) {\
    if( TENCNAMES( venc)[i] == name) {\
      is_present = true;\
    }\
  }\
  if( is_present) {\
    new = SNetTencCreateEmptyVector( TENCNUM( venc) - 1);\
    skip = 0;\
    for( i=0; i<TENCNUM( venc); i++) {\
      if( TENCNAMES( venc)[i] != name) {\
        SNetTencSetVectorEntry( new, i - skip, TENCNAMES( venc)[i]);\
      }\
      else {\
        skip += 1;\
      }\
    }\
    SNetTencDestroyVector( venc->NAMES);\
    venc->NAMES = new;\
  }

#define ADD_TO_VARIANT( GETNUM, GETNAMES, NAMES)\
  int i;\
  snet_vector_t *new_names;\
  for( i=0; i<GETNUM( venc); i++) {\
    if( GETNAMES( venc)[i] == name) {\
      return( false);\
    }\
  }\
  new_names = SNetTencCreateEmptyVector( GETNUM( venc) + 1);\
  for( i=0; i<GETNUM( venc); i++) {\
    SNetTencSetVectorEntry( new_names, i, GETNAMES( venc)[i]);\
  }\
  SNetTencSetVectorEntry( new_names, i, name);\
  SNetTencDestroyVector( venc->NAMES);\
  venc->NAMES = new_names;\
  return( true);




union vfield {
  int  *ints;
  snet_variantencoding_t **v_encodes;
};

struct vector {
  int  num;
  union vfield fields;
};



struct variantencode {
  snet_vector_t *field_names;
  snet_vector_t *tag_names;
  snet_vector_t *btag_names;
};


struct typeencode {
//  snet_vector_t *variants;
  int num;
  snet_variantencoding_t **variants;
};

struct typeencoding_list {
  int num;
  snet_typeencoding_t **types;
};

struct patternencoding {
  int num_tags;
  int *tag_names;
  int *new_tag_names;

  int num_fields;
  int *field_names;
  int *new_field_names;
};

struct patternset {
  int num;
  snet_patternencoding_t **patterns;
};

/* *********************************************************** */

static void DestroyFieldVector( snet_vector_t *vect) {
  if( vect != NULL) {
    SNetMemFree( vect->fields.ints);
  }
  SNetMemFree( vect);         
}



static int *GetIntField( snet_vector_t *vect) {
  if( vect != NULL) {
    return( vect->fields.ints);
  }
  else {
    return( NULL);
  }
}

static snet_vector_t *GetFieldVector( snet_variantencoding_t *v_enc) {
  if( v_enc != NULL) {
    return( v_enc->field_names);
  } 
  else {
    return( NULL);
  }
}

static snet_vector_t *GetTagVector( snet_variantencoding_t *v_enc) {
  if( v_enc != NULL) {
    return( v_enc->tag_names);
  }
  else {
    return( NULL);
  }
}

static snet_vector_t *GetBTagVector( snet_variantencoding_t *v_enc) {
  if( v_enc != NULL) {
    return( v_enc->btag_names);
  }
  else {
    return( NULL);
  }
}

static void Rename( snet_vector_t *vec, int name, int newName) {

  int i;

  i = 0; 
  while( (vec->num > i) &&
         (vec->fields.ints[ i] != name)) {
    i += 1;
  }

  if( i == vec->num) {
    printf("\n\n :: DEBUG INFO :: num: %d, name: %d, new: %d names: ",
            vec->num, name, newName);
    for( i=0; i<vec->num; i++) {
      printf("%d ", vec->fields.ints[i]);
    }
 
    printf("\n\n ** Fatal Error ** : Couldn't find name in vector.\n"
           "                     This is a bug in the runtime system.\n\n");

    exit( 1);
  }

  vec->fields.ints[ i] = newName;
}

/* *********************************************************** */


extern snet_vector_t *SNetTencCreateVector( int num, ...) {

  snet_vector_t *vect;

  TENC_CREATE_VEC( vect, num, int, ints);
  
  return( vect);
}


extern snet_vector_t *SNetTencCreateEmptyVector( int num) {

  int i;
  snet_vector_t *vect;

  vect = SNetMemAlloc( sizeof( snet_vector_t));
  vect->num = num;
  vect->fields.ints = SNetMemAlloc( num * sizeof( int));
  
  for( i=0; i<num; i++) {
    vect->fields.ints[i] = UNSET;
  }

  return( vect);
}


extern void SNetTencSetVectorEntry( snet_vector_t *vect, int num, int val) {

  vect->fields.ints[num] = val;
}

extern int SNetTencGetNumVectorEntries( snet_vector_t *vec) {
  if( vec == NULL) {
    return( -1);
  }
  else {
    return( vec->num);
  }
}

extern void SNetTencRemoveUnsetEntries( snet_vector_t *vec) {

  int num, i;

  num = vec->num;

  for( i=0; i<num; i++) {
    if( vec->fields.ints[i] == UNSET) {
      vec->num -= 1;
    }
  }
}

extern snet_vector_t *SNetTencCopyVector( snet_vector_t *vec) {
  
  int i;
  snet_vector_t *new_vec;

  new_vec = SNetMemAlloc( sizeof( snet_vector_t));
  new_vec->num = vec->num;

  new_vec->fields.ints = SNetMemAlloc( vec->num * sizeof( int));
  for( i=0; i<vec->num; i++) {
    new_vec->fields.ints[i] = vec->fields.ints[i];;
  }

  return( new_vec);
}



extern void SNetTencDestroyVector( snet_vector_t *vec) {
 
  SNetMemFree( vec->fields.ints);
  SNetMemFree( vec);
}

extern int SNetTencGetNumFields( snet_variantencoding_t *v_enc) {

  return( SNetTencGetNumVectorEntries( GetFieldVector(v_enc)));
}

extern int *SNetTencGetFieldNames( snet_variantencoding_t *v_enc) {

  return( GetIntField( GetFieldVector( v_enc))); 
}

extern int SNetTencGetNumTags( snet_variantencoding_t *v_enc) {

  return( SNetTencGetNumVectorEntries( GetTagVector(v_enc)));
}

extern int *SNetTencGetTagNames( snet_variantencoding_t *v_enc) {
  
  return( GetIntField( GetTagVector( v_enc))); 
}

extern int SNetTencGetNumBTags( snet_variantencoding_t *v_enc) {

  return( SNetTencGetNumVectorEntries( GetBTagVector( v_enc)));
}

extern int *SNetTencGetBTagNames( snet_variantencoding_t *v_enc) {
 
  return( GetIntField( GetBTagVector( v_enc))); 
}


extern void SNetTencRenameTag( snet_variantencoding_t *v_enc, int name, int newName) {

  Rename( GetTagVector( v_enc), name, newName); 
}


extern void SNetTencRenameField( snet_variantencoding_t *v_enc, int name, int newName) {

  Rename( GetFieldVector( v_enc), name, newName); 
}


extern void SNetTencRenameBTag( snet_variantencoding_t *v_enc, int name, int newName) {

  Rename( GetBTagVector( v_enc), name, newName); 
}


extern snet_variantencoding_t *SNetTencVariantEncode( snet_vector_t *field_v, snet_vector_t *tag_v, snet_vector_t *btag_v) {

  snet_variantencoding_t *v_encode;

  v_encode = SNetMemAlloc( sizeof( snet_variantencoding_t));

  v_encode->field_names = field_v;
  v_encode->tag_names = tag_v;
  v_encode->btag_names = btag_v;
  
  return( v_encode);
}

extern snet_variantencoding_t 
*SNetTencCopyVariantEncoding( snet_variantencoding_t *venc) {

  return( SNetTencVariantEncode( SNetTencCopyVector( venc->field_names),
                                 SNetTencCopyVector( venc->tag_names),
                                 SNetTencCopyVector( venc->btag_names)));
}

extern void SNetTencDestroyVariantEncoding( snet_variantencoding_t *v_enc) {

  DestroyFieldVector( GetFieldVector( v_enc));
  DestroyFieldVector( GetTagVector( v_enc));
  DestroyFieldVector( GetBTagVector( v_enc));

  SNetMemFree( v_enc);
}

extern bool SNetTencAddTag( snet_variantencoding_t *venc, int name) {
  ADD_TO_VARIANT( SNetTencGetNumTags, SNetTencGetTagNames, tag_names);
}

extern bool SNetTencAddField( snet_variantencoding_t *venc, int name) {
  ADD_TO_VARIANT( SNetTencGetNumFields, SNetTencGetFieldNames, field_names);
}

extern bool SNetTencAddBTag( snet_variantencoding_t *venc, int name) {
  ADD_TO_VARIANT( SNetTencGetNumBTags, SNetTencGetBTagNames, btag_names);
}

extern void SNetTencRemoveTag( snet_variantencoding_t *venc, int name) {
  REMOVE_FROM_TENC( tag_names, SNetTencGetNumTags, SNetTencGetTagNames);
}
extern void SNetTencRemoveBTag( snet_variantencoding_t *venc, int name) {
  REMOVE_FROM_TENC( btag_names, SNetTencGetNumBTags, SNetTencGetBTagNames);
}
extern void SNetTencRemoveField( snet_variantencoding_t *venc, int name) {
  REMOVE_FROM_TENC( field_names, SNetTencGetNumFields, SNetTencGetFieldNames);
}


extern snet_typeencoding_t *SNetTencTypeEncode( int num, ...) {

  int i;
  va_list args;
 
  snet_typeencoding_t *t_encode;

  t_encode = SNetMemAlloc( sizeof( snet_typeencoding_t));
  t_encode->variants = SNetMemAlloc( num * sizeof( snet_variantencoding_t*));
  t_encode->num = num;

  va_start(args, num);
  
  for( i=0; i < num; i++) {
    (t_encode->variants)[i] = va_arg(args, snet_variantencoding_t*);
  }
  va_end( args);
  
  return( t_encode);
}

extern snet_typeencoding_t 
*SNetTencAddVariant( snet_typeencoding_t *t,
                     snet_variantencoding_t *v)
{
  int i;
  snet_variantencoding_t **variants;

  variants = SNetMemAlloc( (t->num + 1) * sizeof( snet_variantencoding_t*));
  for( i=0; i<t->num; i++) {
    variants[i] = t->variants[i];
  } 
  variants[t->num] = v;
  t->num += 1;

  SNetMemFree( t->variants);
  t->variants = variants;

  return( t);
}


extern snet_typeencoding_list_t 
*SNetTencCreateTypeEncodingListFromArray( int num, snet_typeencoding_t **t) {

  snet_typeencoding_list_t *lst;
  if( num == 0) {
    lst = NULL;
  }
  else {
    lst = SNetMemAlloc( sizeof( snet_typeencoding_list_t));
    lst->types = t;
    lst->num = num;
  }

  return( lst);  
}



extern snet_typeencoding_list_t 
*SNetTencCreateTypeEncodingList( int num, ...) {

  int i;
  va_list args;
  snet_typeencoding_t **t;
  
  t = SNetMemAlloc( num * sizeof( snet_typeencoding_t*));

  va_start(args, num);
  
  for( i=0; i < num; i++) {
    t[i] = va_arg(args, snet_typeencoding_t*);
  }
  va_end( args);
  
  return( SNetTencCreateTypeEncodingListFromArray( num, t));
}




extern int SNetTencGetNumTypes( snet_typeencoding_list_t *lst) {
    return( lst->num);
}
extern snet_typeencoding_t *SNetTencGetTypeEncoding( snet_typeencoding_list_t *lst, int num) {
    return( lst->types[num]);
}



extern int SNetTencGetNumVariants( snet_typeencoding_t *type) {
  int num;

  num = type->num;

  return( num);
}

extern snet_variantencoding_t *SNetTencGetVariant( snet_typeencoding_t *type, int num) {
  if( type->num < 1) {
    return( NULL);
  }
  else {
    return( type->variants[num-1]);  
  }
}

extern void SNetDestroyTypeEncoding( snet_typeencoding_t *t_enc) {

  int i;

  for( i=0; i<t_enc->num; i++) {
    SNetTencDestroyVariantEncoding( ( t_enc->variants)[i]);
  }
  SNetMemFree( t_enc->variants);
  SNetMemFree( t_enc);
}



extern snet_patternencoding_t *SNetTencPatternEncode( int num_tags, int num_fields, ...) {

  int i;
  va_list args;
  snet_patternencoding_t *pat;

  pat = SNetMemAlloc( sizeof( snet_patternencoding_t));
  pat->num_tags = num_tags;
  pat->num_fields = num_fields;
  pat->tag_names = SNetMemAlloc( num_tags * sizeof( int));
  pat->new_tag_names = SNetMemAlloc( num_tags * sizeof( int));
  pat->field_names = SNetMemAlloc( num_fields * sizeof( int));
  pat->new_field_names = SNetMemAlloc( num_fields * sizeof( int));

  va_start( args, num_fields);

  for( i=0; i<num_tags; i++) {
    pat->tag_names[i] = va_arg(args, int);
    pat->new_tag_names[i] = va_arg( args, int);
  }
  for( i=0; i<num_fields; i++) {
   pat->field_names[i] = va_arg( args, int);
   pat->new_field_names[i] = va_arg( args, int);
  }

  va_end( args);

  return( pat);
}


extern int SNetTencGetNumPatternTags( snet_patternencoding_t *pat) {
  return( pat->num_tags);
}
extern int *SNetTencGetPatternTagNames( snet_patternencoding_t *pat) {
  return( pat->tag_names);
}

extern int *SNetTencGetPatternNewTagNames( snet_patternencoding_t *pat) {
  return( pat->new_tag_names);
}

extern int SNetTencGetNumPatternFields( snet_patternencoding_t *pat) {
  return( pat->num_fields);
}

extern int *SNetTencGetPatternFieldNames( snet_patternencoding_t *pat) {
  return( pat->field_names);
}

extern int *SNetTencGetPatternNewFieldNames( snet_patternencoding_t *pat) {
  return( pat->new_field_names);
}

extern void TENCdestroyPatternEncode( snet_patternencoding_t *pat) {
  
  SNetMemFree( pat->tag_names); SNetMemFree( pat->new_tag_names);
  SNetMemFree( pat->field_names); SNetMemFree( pat->new_field_names);
  SNetMemFree( pat);
}

extern snet_patternset_t *SNetTencCreatePatternSet( int num, ...) {

  int i;
  va_list args;
  snet_patternset_t *patset;

  patset = SNetMemAlloc( sizeof( snet_patternset_t));
  patset->patterns = SNetMemAlloc( num * sizeof( snet_patternencoding_t*));
  patset->num = num;

  va_start( args, num);
  for( i=0; i<num; i++) {
    patset->patterns[i] = va_arg(args, snet_patternencoding_t*);
  }
  va_end( args);

  return( patset);
}


extern int SNetTencGetNumPatterns( snet_patternset_t *patterns) {
  return( patterns->num);
}


extern snet_patternencoding_t *SNetTencGetPatternEncoding( snet_patternset_t *patset, int num) {

  return( patset->patterns[num]);
}

extern void SNetTencDestroyPatternSet( snet_patternset_t *patset) {

  SNetMemFree( patset->patterns);
  SNetMemFree( patset);
}









