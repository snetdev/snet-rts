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

 extern int yylex(void);
 extern void yylex_destroy();
 void yyerror(char *error);
 extern void yyrestart(FILE *);
 extern FILE *yyin;

 /***** Struct definitions ******/

 /* Struct of two strings for a lists */
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
  int              cint;
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
		attrib_t *attr = searchAttribute($2, MODE);

		/* Default mode: */
		current.mode = MODE_BINARY;

		if(attr != NULL) {
		  if(strcmp(attr->value, TEXTUAL) == 0) {
		    current.mode = MODE_TEXTUAL;
		  }
		  else if(strcmp(attr->value, BINARY) == 0) {
		    current.mode = MODE_BINARY;
		  }
		}
              }
              TAG_END Records DATA_END_BEGIN TAG_END{
		deleteAttributes($2);
              }
            ;

Records:      Record Records
              {

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

		    if(interface != NULL) {
		      current.record = SNetRecCreate(REC_data,
					   SNetTencVariantEncode( 
					       SNetTencCreateVector( 0), 
					       SNetTencCreateVector( 0), 
					       SNetTencCreateVector( 0)));

		      SNetRecSetInterfaceId(current.record, 
					    SNetInInterfaceToId(parser.interface, 
								interface->value));
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
		    // TODO: record with unknown type!
		  }
		}
		else{
		  yyerror("Record with no type found!");
		  // TODO: ERROR 
		}

		if(current.record != NULL) {
		  SNetBufPut(parser.buffer, current.record);
		  current.record = NULL;
		}

		deleteAttributes($2);
	      }

            | RECORD_BEGIN Attributes 
              { /* MID-RULE: */
		/* Form record and get default interface! */
		attrib_t *attr = searchAttribute($2, TYPE);
		if(attr != NULL) {
		  if(strcmp(attr->value, SNET_REC_DATA) == 0) {

		    current.record = SNetRecCreate(REC_data,
					 SNetTencVariantEncode( 
					     SNetTencCreateVector( 0), 
					     SNetTencCreateVector( 0), 
					     SNetTencCreateVector( 0)));
		    
		    attrib_t *interface = searchAttribute($2, INTERFACE);
		    if(interface != NULL) {	    
		      SNetRecSetInterfaceId(current.record, 
					    SNetInInterfaceToId(parser.interface, 
								interface->value));
		    }else {
		      SNetRecSetInterfaceId(current.record, SNET_INTERFACE_ERROR);
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
		    // TODO: record with unknown type!
		  }
		}
		else{
		  // TODO: ERROR 
		}
	      } 
              TAG_END Entitys RECORD_END_BEGIN TAG_END{

		if(current.record != NULL) {

		  if(SNetRecGetDescriptor(current.record) == REC_data
		     && SNetRecGetInterfaceId(current.record) == SNET_INTERFACE_ERROR) {
		    SNetRecDestroy(current.record);
		  } else {
		    SNetBufPut(parser.buffer, current.record);
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
        | /**/
          {
	  }
        ;


Field:    FIELD_BEGIN Attributes STARTTAG_SHORTEND
          {
	    deleteAttributes($2);
	  }
        | FIELD_BEGIN Attributes TAG_END
          { /* MID-RULE: */

	    attrib_t *attr = NULL;
	    void *data = NULL;
	    int iid;
	    attrib_t *finterface;
	    int label;

	    attr = searchAttribute($2, LABEL);
	    if(attr != NULL) {
	      label = SNetInLabelToId(parser.labels, attr->value);
	    }

	    iid = SNetRecGetInterfaceId(current.record);

	    finterface = searchAttribute($2, INTERFACE);

	    if(finterface != NULL) {
	      int i = SNetInInterfaceToId(parser.interface, finterface->value);

	      if(iid == SNET_INTERFACE_ERROR) {
		SNetRecSetInterfaceId(current.record, i);
	      }

	      iid = i;
	    }

	    // TODO: test for mode!

	    void *(*desfun)(FILE *) = SNetGetDeserializationFun(iid);

	    if(desfun != NULL) {
	      data = desfun(yyin);
	    }

	    SNetRecAddField(current.record, label);
	    SNetRecSetField(current.record, label, data);

	    yyrestart(yyin);
	    
	    
          }
          FIELD_END_BEGIN TAG_END
          {
	  
	    deleteAttributes($2);
          }
        ;

Tag:      TAG_BEGIN Attributes STARTTAG_SHORTEND
          {
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
	      SNetRecSetTag(current.record, label, atoi($4));
	    }

	    deleteAttributes($2);
          }
        ;

Btag:     BTAG_BEGIN Attributes STARTTAG_SHORTEND
          {
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
	      SNetRecSetBTag(current.record, label, atoi($4));
	    }

	    deleteAttributes($2);
          }
        ;

Attributes:   NAME EQ SQUOTE SATTVAL SQUOTE Attributes 
              {  
         
		/* Default namespace */
		if(strcmp($1, "xmlns") == 0){
		  if(strcmp($4, SNET_NAMESPACE) != 0) {
		    // TODO: Wrong name space!
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
		    // TODO: Wrong name space!
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

void yyerror(char *error) 
{
  printf("\nParse error: %s\n", error);
  parser.terminate = SNET_PARSE_ERROR;
}

void SNetInParserInit(snetin_label_t *labels,
		      snetin_interface_t *interfaces,
		      snet_buffer_t *in_buf)
{  
  parser.labels = labels;
  parser.interface = interfaces;
  parser.buffer = in_buf;

  parser.terminate = SNET_PARSE_CONTINUE;
}  

static void flush()
{
  if(current.record != NULL) {
    SNetRecDestroy(current.record);
    current.record = NULL;
  }
  current.mode = MODE_BINARY;
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
