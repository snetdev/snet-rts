/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   14.12.2007
 * -----
 *
 * Description:
 * ------------
 *
 * Parser of S-NET net interface.
 *
 *******************************************************************************/

%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "label.h"
#include "interface.h"
#include "memfun.h"
#include "snetentities.h"

#define SNET_NAMESPACE "snet-home.org"

/* SNet record types */
#define SNET_REC_DATA       "data" 
#define SNET_REC_SYNC       "sync"
#define SNET_REC_COLLECT    "collect"
#define SNET_REC_SORT_BEGIN "sort_begin"
#define SNET_REC_SORT_END   "sort_end"
#define SNET_REC_TERMINATE  "terminate"

#define LABEL     "label"
#define INTERFACE "interface"
#define TYPE      "type"
#define MODE      "mode"
#define TEXTUAL   "textual"
#define BINARY    "binary"

#define MODE_BINARY 0
#define MODE_TEXTUAL 1
#define INTERFACE_UNKNOWN -1

 extern int yylex(void);
 extern void yylex_destroy();
 void yyerror(char *error);
 extern void yyrestart(FILE *);
 extern FILE *yyin;

 /***** Struct definitions ******/

 /* Struct to represent xml attribute. */
 typedef struct attribute{
   char *name;
   char *value;
   struct attribute *next;
 } attrib_t;

 /***** Globals variables *****/
 static struct{

   /* Value that tells if a terminate record has been encounterd in the input stream */
   unsigned int terminate;  

   /* Labels to use with incoming fields/tags/btags */
   snetin_label_t *labels;

   /* Interfaces to use with incoming fields. */
   snetin_interface_t *interface;

   /* Buffer where all the parsed data should be put */
   snet_buffer_t *buffer;
 }parser;

 /* Data values for record currently under parsing  */
 static struct{
   snet_record_t *record;
   int interface;
   int mode;
 }current;
 
 /** Attribute functions **
  *
  * These functions are used to handle XML-attributes in
  * the parser.
  *
  */

 /* Init XML-attribute names "name" with value "value" */
 static attrib_t *initAttribute(char *name, char *value){
   attrib_t *attrib = SNetMemAlloc(sizeof(attrib_t));

   attrib->name = name;

   attrib->value = value;

   attrib->next = NULL;
   return attrib;
 }

 /* Delete XML attribute */
 static void deleteAttribute(attrib_t *attrib){
   if(attrib != NULL){
     if(attrib->name != NULL){
       SNetMemFree(attrib->name);
     }
     if(attrib->value != NULL){
       SNetMemFree(attrib->value);
     }
     SNetMemFree(attrib);
   }
 }

 static void deleteAttributes(attrib_t *attrib){
   attrib_t *next = NULL;
  
   while(attrib != NULL) {
     next = attrib->next;
     deleteAttribute(attrib);
     attrib = next;
   }
 }

 /* Search XML-attribute list for attribute with given name
  * The first one that matches is returned  
  */
 static attrib_t *searchAttribute(attrib_t *ats, const char *name){
   if(name != NULL){
     attrib_t *temp = ats;
     while(temp != NULL){
       if(strcmp(temp->name, name) == 0){
	 return temp;
       }
       temp = temp->next;
     }
   }
   return NULL;
 }

%}

%union {
  char             *str;
  struct attribute *attrib;
}

%token STARTTAG_SHORTEND DQUOTE SQUOTE TAG_END DATA_END_BEGIN RECORD_END_BEGIN FIELD_END_BEGIN
%token TAG_END_BEGIN BTAG_END_BEGIN DATA_BEGIN RECORD_BEGIN FIELD_BEGIN TAG_BEGIN BTAG_BEGIN
%token EQ VERSION ENCODING STANDALONE YES NO VERSIONNUMBER XMLDECL_END XMLDECL_BEGIN
 
%token <str>  NAME CHARDATA DATTVAL SATTVAL ENCNAME 
%type <attrib> Attributes

%start Document

%%

Document:     Prolog Data 
{
		/* YYACCEPT is needed to stop the parsing,
		   as there might be more characters in the
		   input stream! */
  
		YYACCEPT;
              }


/* At the moment these values are just ignored */ 
Prolog:       XMLDecl
              {

              }
            | /* EMPTY */
              {

              }
            ;


/* At the moment these values are just ignored */
XMLDecl:      XMLDECL_BEGIN VersionInfo EncodingDecl SDDecl XMLDECL_END
            | XMLDECL_BEGIN VersionInfo EncodingDecl XMLDECL_END
            | XMLDECL_BEGIN VersionInfo SDDecl XMLDECL_END
            | XMLDECL_BEGIN VersionInfo XMLDECL_END
              {

              }
            ;

/* At the moment these values are just ignored */
VersionInfo:  VERSION EQ DQUOTE VERSIONNUMBER DQUOTE
            | VERSION EQ SQUOTE VERSIONNUMBER SQUOTE
              {

              }
            ;
/* At the moment these values are just ignored */
EncodingDecl: ENCODING EQ DQUOTE ENCNAME DQUOTE
              {
		if($4 != NULL){
		  SNetMemFree($4);
		}
              }
            | ENCODING EQ SQUOTE ENCNAME SQUOTE 
              {
		if($4 != NULL){
		  SNetMemFree($4);
		}
              }
            ;

/* At the moment these values are just ignored */
SDDecl:       STANDALONE EQ SQUOTE YES SQUOTE
            | STANDALONE EQ SQUOTE NO SQUOTE
            | STANDALONE EQ DQUOTE YES DQUOTE 
            | STANDALONE EQ DQUOTE NO DQUOTE
              {

              }
            ;

Data:         DATA_BEGIN Attributes STARTTAG_SHORTEND
              {
		/* Empty data. This can be ignored! */
		deleteAttributes($2);
	      }
            | DATA_BEGIN Attributes 
              { /* MID-RULE: */
		//	attrib_t *attr = searchAttribute($2, MODE);

		/* Default mode: */
		/*current.mode = MODE_BINARY;

		if(attr != NULL) {
		if(strcmp(attr->value, TEXTUAL) == 0) {
		current.mode = MODE_TEXTUAL;
		}
		else if(strcmp(attr->value, BINARY) == 0) {
		current.mode = MODE_BINARY;
		}
		}*/
              }

              TAG_END Records DATA_END_BEGIN TAG_END
              {
		deleteAttributes($2);
              }
            ;

Records:      Record Records
              {
		/* List of records */
	      }
            | /* EMPTY*/
              {
          
              }
            ;

Record:       RECORD_BEGIN Attributes STARTTAG_SHORTEND
              {
		attrib_t *attr = searchAttribute($2, TYPE);
		if(attr != NULL) {

		  /* Data record: */
		  if(strcmp(attr->value, SNET_REC_DATA) == 0) {

		    attrib_t *interface = searchAttribute($2, INTERFACE);
		    attrib_t *mode = searchAttribute($2, MODE);

		    /* Default mode: */
		    current.mode = MODE_BINARY;

		    if(mode != NULL) {
		      if(strcmp(mode->value, TEXTUAL) == 0) {
			current.mode = MODE_TEXTUAL;
		      }
		      else if(strcmp(mode->value, BINARY) == 0) {
			current.mode = MODE_BINARY;
		      }
		    }
		  
		    if(interface != NULL) {
		      current.record = SNetRecCreate(REC_data,
					   SNetTencVariantEncode( 
					       SNetTencCreateVector( 0), 
					       SNetTencCreateVector( 0), 
					       SNetTencCreateVector( 0)));

		      current.interface = SNetInInterfaceToId(parser.interface, interface->value);
		      SNetRecSetInterfaceId(current.record, current.interface);

		      if(current.mode == MODE_BINARY) {
			SNetRecSetDataMode(current.record, MODE_binary);
		      }
		      else {
			SNetRecSetDataMode(current.record, MODE_textual);
		      }
		    }

		  }else if(strcmp(attr->value, SNET_REC_SYNC) == 0){
		    // TODO: What additional data is needed?
		    current.record = SNetRecCreate(REC_sync);
		  }else if(strcmp(attr->value, SNET_REC_COLLECT) == 0){
		    // TODO: What additional data is needed?
		    current.record = SNetRecCreate(REC_collect);
		  }else if(strcmp(attr->value, SNET_REC_SORT_BEGIN) == 0){
		    // TODO: What additional data is needed?
		    current.record = SNetRecCreate(REC_sort_begin);
		  }else if(strcmp(attr->value, SNET_REC_SORT_END) == 0){
		    // TODO: What additional data is needed?
		    current.record = SNetRecCreate(REC_sort_end);
		  
		  }else if(strcmp(attr->value, SNET_REC_TERMINATE) == 0){
		   
		    current.record = SNetRecCreate(REC_terminate);
		    if(parser.terminate != SNET_PARSE_ERROR){
		      parser.terminate = SNET_PARSE_TERMINATE;
		    }
		    
		  }else{
		    yyerror("Record with unknown type found!");
		  }
		}
		else{
		  yyerror("Record without type found!");
		}

		if(parser.terminate != SNET_PARSE_ERROR) {
		  if(current.record != NULL) {
		    SNetBufPut(parser.buffer, current.record);
		    current.record = NULL;
		    current.interface = INTERFACE_UNKNOWN;
		  }
		}else {
		  /* Discard the record because of parsing error. */
		  SNetRecDestroy(current.record);
		  current.record = NULL;
		  current.interface = INTERFACE_UNKNOWN;
		  yyerror("Error encountered while parsing a record. Record discarded!");
		  parser.terminate = SNET_PARSE_CONTINUE;
		}

		deleteAttributes($2);
	      }

            | RECORD_BEGIN Attributes 
              { /* MID-RULE: */
		/* Form record and get default interface! */
		attrib_t *attr = searchAttribute($2, TYPE);
		if(attr != NULL) {
		  if(strcmp(attr->value, SNET_REC_DATA) == 0) {

		    attrib_t *interface = searchAttribute($2, INTERFACE);
		    attrib_t *mode = searchAttribute($2, MODE);

		    current.record = SNetRecCreate(REC_data,
					 SNetTencVariantEncode( 
					     SNetTencCreateVector( 0), 
					     SNetTencCreateVector( 0), 
					     SNetTencCreateVector( 0)));


		    /* Default mode: */
		    current.mode = MODE_BINARY;

		    if(mode != NULL) {
		      if(strcmp(mode->value, TEXTUAL) == 0) {
			current.mode = MODE_TEXTUAL;
		      }
		      else if(strcmp(mode->value, BINARY) == 0) {
			current.mode = MODE_BINARY;
		      }
		    }
	
		    if(current.mode == MODE_BINARY) {
		      SNetRecSetDataMode(current.record, MODE_binary);
		    }
		    else {
		      SNetRecSetDataMode(current.record, MODE_textual);
		    }
		    
		    if(interface != NULL) {
		      current.interface = SNetInInterfaceToId(parser.interface, 
							      interface->value);
		      SNetRecSetInterfaceId(current.record, current.interface);
		    }else {
		      SNetRecSetInterfaceId(current.record, INTERFACE_UNKNOWN);
		    }

		  }else if(strcmp(attr->value, SNET_REC_SYNC) == 0){
		    // TODO: What additional data is needed?
		    current.record = SNetRecCreate(REC_sync);
		  }else if(strcmp(attr->value, SNET_REC_COLLECT) == 0){
		    // TODO: What additional data is needed?
		    current.record = SNetRecCreate(REC_collect);
		  }else if(strcmp(attr->value, SNET_REC_SORT_BEGIN) == 0){
		    // TODO: What additional data is needed?
		    current.record = SNetRecCreate(REC_sort_begin);
		  }else if(strcmp(attr->value, SNET_REC_SORT_END) == 0){
		    // TODO: What additional data is needed?
		    current.record = SNetRecCreate(REC_sort_end);
		  }else if(strcmp(attr->value, SNET_REC_TERMINATE) == 0){
		    /* New control record: terminate */
		    current.record = SNetRecCreate(REC_terminate);
		    if(parser.terminate != SNET_PARSE_ERROR){
		      parser.terminate = SNET_PARSE_TERMINATE;
		    }
		  }else{
		    yyerror("Record with unknown type found!");
		  }
		}else{
		    yyerror("Record without type found!");
		}
	      } 
              TAG_END Entitys RECORD_END_BEGIN TAG_END{

		if(current.record != NULL) {

		  if(parser.terminate != SNET_PARSE_ERROR) {
		    if(SNetRecGetDescriptor(current.record) == REC_data
		       && SNetRecGetInterfaceId(current.record) == INTERFACE_UNKNOWN) {
		      SNetRecDestroy(current.record);
		      current.record = NULL;
		      current.interface = INTERFACE_UNKNOWN;
		      yyerror("Error encountered while parsing a record. Record discarded!");
		      parser.terminate = SNET_PARSE_CONTINUE;
		    } else {
		      SNetBufPut(parser.buffer, current.record);
		      current.record = NULL;
		      current.interface = INTERFACE_UNKNOWN;
		    }
		  }else {
		    /* Discard the record because of parsing error. */
		    SNetRecDestroy(current.record);
		    current.record = NULL;
		    current.interface = INTERFACE_UNKNOWN;
		    yyerror("Error encountered while parsing a record. Record discarded!");
		    parser.terminate = SNET_PARSE_CONTINUE;
		  }
		  
		  current.record = NULL;
		}

		deleteAttributes($2);

              }
          ;

Entitys:  Field Entitys
          {

	  }
        | Tag Entitys
          {

	  }
        | Btag Entitys
          {

	  }
        | /* EMPTY */
          {
	  }
        ;


Field:    FIELD_BEGIN Attributes STARTTAG_SHORTEND
          {
	    /* Field without any data. 
	     * TODO: This is an error! */
	    yyerror("Field without data encountered!");
	    deleteAttributes($2);
	  }
        | FIELD_BEGIN Attributes TAG_END
          { /* MID-RULE: */

	    attrib_t *attr = NULL;
	    void *data = NULL;
	    int iid;
	    attrib_t *finterface;
	    int label;
	    void *(*fun)(FILE *);

	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    } else{
	      yyerror("Field without label found!");
	    }

	    finterface = searchAttribute($2, INTERFACE);

	    if(finterface == NULL) {
	      /* This can be INTERFACE_UNKNOWN!*/
	      iid = current.interface;
	    }else {
	      int i = SNetInInterfaceToId(parser.interface, finterface->value);
	      
	      if(current.interface != INTERFACE_UNKNOWN) {
		if(i != current.interface) {
		  /* The record and the field have different interfaces! */
		  yyerror("Interface error!");
		  i = INTERFACE_UNKNOWN;
		}
	      }else {
		if(i == SNET_INTERFACE_ERROR) {
		  /* The field has unknown interface! */
		  yyerror("Interface error!");
		  i = INTERFACE_UNKNOWN;
		}else {
		  /* No default interface declared. The record might have
		   * interface if this is not the first field. */
		  iid = SNetRecGetInterfaceId(current.record);

		  if(iid == INTERFACE_UNKNOWN) {
		    /* No interface set yet. This field dictates the interface. */
		    SNetRecSetInterfaceId(current.record, i);
		    iid = i;
		  } else if(iid != i) {
		    /* The record and the field have different interfaces! */
		    yyerror("Interface error!");
		    i = INTERFACE_UNKNOWN;
		  }
		}
	      }
	      
	      iid = i;
	    }

	    if(iid != INTERFACE_UNKNOWN) {
	      if(current.mode == MODE_TEXTUAL) {
		fun = SNetGetDeserializationFun(iid);
	      } else if(current.mode == MODE_BINARY) {
		fun = SNetGetDecodingFun(iid);
	      } else {
		yyerror("Unknown data mode");
	      }

	      if(fun != NULL) {
		data = fun(yyin);
	     
		if(data != NULL) {
		  SNetRecAddField(current.record, label);
		  SNetRecSetField(current.record, label, data);
		} else {
		  yyerror("Could not decode data!");
		}

		while(getc(yyin) != '<');		  
		if(ungetc('<', yyin) == EOF){
		  /* TODO: This is an error. First char of the next tag is already consumed! */
		}
	      }
	      yyrestart(yyin);
	    }else { 
	      /* If we cannot deserialise the data we must ignore it! */
	     
	      while(getc(yyin) != '<');
	      if(ungetc('<', yyin) == EOF){
		/* TODO: This is an error. First char of the next tag is already consumed! */
	      }
	    }
          }
          FIELD_END_BEGIN TAG_END
          {
	  
	    deleteAttributes($2);
          }
        ;

Tag:      TAG_BEGIN Attributes STARTTAG_SHORTEND
          {
	    /* Tag with no value. TODO: This is an error? Or short for 0? */
	    yyerror("Tag without data encountered!");
	    deleteAttributes($2);
	  }
        | TAG_BEGIN Attributes TAG_END CHARDATA TAG_END_BEGIN TAG_END
          {
	    attrib_t *attr = NULL;
	    int label;
	    
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    
	      SNetRecAddTag(current.record, label);

	      /* TODO: test that the atoi call worked? */
	      SNetRecSetTag(current.record, label, atoi($4));
	    } else{
	      yyerror("Tag without label found!");
	    }


	    deleteAttributes($2);
          }
        ;

Btag:     BTAG_BEGIN Attributes STARTTAG_SHORTEND
          {
	    /* Btag with no value. TODO: This should be an error? or short for 0?*/
	    yyerror("Btag without data encountered!");
	    deleteAttributes($2);
	  }
        | BTAG_BEGIN Attributes TAG_END CHARDATA BTAG_END_BEGIN TAG_END
          {
	    attrib_t *attr = NULL;
	    int label;
	    
	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    
	      SNetRecAddBTag(current.record, label);

	      /* TODO: test that the atoi call worked correctly? */
	      SNetRecSetBTag(current.record, label, atoi($4));
	    } else{
	      yyerror("Btag without label found!");
	    }

	    deleteAttributes($2);
          }
        ;

Attributes:   NAME EQ SQUOTE SATTVAL SQUOTE Attributes 
              {  
         
		/* Default namespace */
		if(strcmp($1, "xmlns") == 0){
		  if(strcmp($4, SNET_NAMESPACE) != 0) {
		    // TODO: Wrong name space! Do we even want to check this?
		    yyerror("Data in wrong namespace!");
		  }
		  SNetMemFree($4);
		  SNetMemFree($1);
		  $$ = $6;
		}/* Normal attribute */
		else{
		  attrib_t *temp = initAttribute($1, $4);
		  temp->next = $6;
		  $$ = temp;
		}
              }

            | NAME EQ DQUOTE DATTVAL DQUOTE Attributes 
              {  
		/* Default namespace */
		if(strcmp($1, "xmlns") == 0){
		  if(strcmp($4, SNET_NAMESPACE) != 0) {
		    // TODO: Wrong name space! Do we even want to check this?
		    yyerror("Data in wrong namespace!");
		  }
		  SNetMemFree($4);
		  SNetMemFree($1);
		  $$ = $6;
		}/* Normal attribute */
		else{
		  attrib_t *temp = initAttribute($1, $4);
		  temp->next = $6;;
		  $$ = temp;
		}
              }

            | /* EMPTY */
              {                
                $$ = NULL;
              }

%%

static void flush()
{
  if(current.record != NULL) {
    SNetRecDestroy(current.record);
    current.record = NULL;
  }

  current.interface = INTERFACE_UNKNOWN;

  current.mode = MODE_BINARY;

  if(parser.terminate == SNET_PARSE_ERROR) {
    parser.terminate = SNET_PARSE_CONTINUE;
  }
}

void yyerror(char *error) 
{
  if(error != NULL && strcmp(error, "syntax error") != 0) {
    printf("\n  ** Parse error: %s\n\n", error);
  }

  if(parser.terminate != SNET_PARSE_TERMINATE) {
    parser.terminate = SNET_PARSE_ERROR;
  }
}

void SNetInParserInit(FILE *file,
		      snetin_label_t *labels,
		      snetin_interface_t *interfaces,
		      snet_buffer_t *in_buf)
{  
  yyin = file; 
  parser.labels = labels;
  parser.interface = interfaces;
  parser.buffer = in_buf;

  parser.terminate = SNET_PARSE_CONTINUE;
}  

int SNetInParserParse()
{
  flush();

  if(parser.terminate == SNET_PARSE_CONTINUE) {
    yyparse();
  }
 
  return parser.terminate;
}

void SNetInParserDestroy()
{
  yylex_destroy();
}

#undef SNET_NAMESPACE 
#undef SNET_REC_DATA     
#undef SNET_REC_SYNC       
#undef SNET_REC_COLLECT    
#undef SNET_REC_SORT_BEGIN 
#undef SNET_REC_SORT_END   
#undef SNET_REC_TERMINATE  
#undef LABEL     
#undef INTERFACE 
#undef TYPE    
#undef MODE      
#undef TEXTUAL  
#undef BINARY    
#undef MODE_BINARY
#undef MODE_TEXTUAL
#undef INTERFACE_UNKNOWN
