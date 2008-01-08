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
#include <memfun.h>
#include <snetentities.h>

/*** This might change according to the interface */
#include <C2SNet.h>
/**************************************************/

#include "parser.h"
#include "str.h"

/* Values to define state of the parser (in which kind of XML element) */
#define UNKNOWN           0
#define ROOT              1
#define DATA              2
#define RECORD            3
#define RECORD_DATA       4
#define RECORD_SYNC       5
#define RECORD_COLLECT    6
#define RECORD_SORT_BEGIN 7
#define RECORD_SORT_END   8
#define RECORD_TERMINATE  9
#define FIELD             10
#define TAG               11
#define BTAG              12

/* Values to define mode of the data */
#define MODE_UNKNOWN 0
#define MODE_TEXTUAL 1
#define MODE_BINARY  2

/* Normalized XML element names */
#define SNET_NS_DATA     "snet.feis.herts.ac.uk:data"
#define SNET_NS_RECORD   "snet.feis.herts.ac.uk:record"
#define SNET_NS_FIELD    "snet.feis.herts.ac.uk:field"
#define SNET_NS_TAG      "snet.feis.herts.ac.uk:tag"
#define SNET_NS_BTAG     "snet.feis.herts.ac.uk:btag"

/* SNet record types */
#define SNET_REC_DATA       "data" 
#define SNET_REC_SYNC       "sync"
#define SNET_REC_COLLECT    "collect"
#define SNET_REC_SORT_BEGIN "sort_begin"
#define SNET_REC_SORT_END   "sort_end"
#define SNET_REC_TERMINATE  "terminate"


/* SNet attribute names and values */
#define LABEL     "label"
#define INTERFACE "interface"
#define MODE      "mode"
#define TEXTUAL   "textual"
#define BINARY    "binary"
#define TYPE      "type"

/* SNet interface names */
#define INTERFACE_C2SNET "C2SNet"

/** Interface values */
#define INTERFACE_NUM_UNKNOWN -1
#define INTERFACE_NUM_C2SNET   0

 extern int yylex(void);
 extern void yylex_destroy();

 void yyerror(char *error);




 /***** Struct definitions ******/
 
 /* Struct of two strings for lists */
 typedef struct pair{
   char *name;
   char *value;
   struct pair *next;
 }pair_t;

 /* Struct for XML attribute */
 typedef struct pair attrib_t;

 /* Struct for bound namespaces */
 typedef struct pair ns_t;

 /* Struct for namespace stack */
 typedef struct nsStack{
   char *deflt; // Default namespace in this node.
   ns_t *bound; // Namespaces that are bound to some name visible in this node.
   struct nsStack *next;
 }nsStack_t;

 /* Struct for parser state stack */
 typedef struct stateStack{
   int state; 
   struct stateStack *next;
 } state_t;




 /***** Global variables *****/
 struct{
   /* Labels for SNet fields/tags/btags and their mapping to numbers */
   label_t *labels;; 

   /* Value that tells if a terminate record has been encounterd in the input stream */
   unsigned int terminate;  

   /* Data values for record currently under parsing  */
   struct{
     snet_record_t *record;

     /* Mode of the data in the record */
     unsigned int mode;

     /* Label of the fiel/tag/btag that is currently under parsing */
     char *label;

     /* Interface of the field that is currently under parsing */
     int interface;
   }current;

   /* Buffer where all the parsed data should be put */
   snet_buffer_t *buffer;
   
   /* functions for data copying, freeing and deserializing */
   void *(*func_deserialize)(const char*); 
   void (*func_free)( void*); 
   void* (*func_copy)( void*); 
  
   /* All the bound namespaces */
   ns_t *bound_ns; 

   /* Namespace stack for the parser */
   nsStack_t *ns_stack;
 
   /* State stack for the parser */
   state_t *state_stack;
 }parser;




 /***** Auxiliary functions *****/

 /* Init new bound namespace with prefix bound to name*/
 static void initNS(char *prefix, char *name){
   ns_t *ns = SNetMemAlloc(sizeof(ns_t));

   ns->name = prefix;

   ns->value = name;

   ns->next = parser.bound_ns;
   parser.bound_ns = ns;
 }

 /* Delete namespace binding */
 static void deleteNS(ns_t *ns){
   if(ns != NULL){
     if(ns->value != NULL){
       SNetMemFree(ns->value);
     }
     if(ns->name != NULL){
       SNetMemFree(ns->name);
     }
     SNetMemFree(ns);
   }
 }

 /* Get default namespace (on the top namespace stack) */
 static char *getDefaultNS(){
   return parser.ns_stack->deflt;
 }

 /* Set the default namespace on the top of current namespace stack */
 static void setDefaultNS(char *ns){
   nsStack_t *def = parser.ns_stack;
   if(def->next != NULL){
     if(def->deflt != def->next->deflt){
       SNetMemFree(def->deflt);
     }
   }

   def->deflt = ns;
 }

 /* Search for namespace bound to given prefix*/
 static char *searchNS(const char *prefix, int len){
   if(prefix != NULL){
     ns_t *ns = parser.bound_ns;
     while(ns != NULL){
       if(strncmp(ns->name, prefix, len) == 0){
	 return ns->value;
       }
       ns = ns->next;
     }
   }
   return NULL;
 }



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

 /* Search XML-attribute list for attribute with given name
  * The first one that matches is returned  
  */
 static attrib_t *searchAttribute(attrib_t *ats, const char *name){
   if(name != NULL){
     attrib_t *temp = ats;
     while(temp != NULL){
       if(STRcmp(temp->name, name) == 0){
	 return temp;
       }
       temp = temp->next;
     }
   }
   return NULL;
 }

 /* Normalize XML-element name.
  * Normalizing means that the namespaze of the element is added
  * before it's name.
  *
  * For example:
  * if current namespace is "snet.feis.herts.ac.uk" and the element 
  * name is "record", name "snet.feis.herts.ac.uk:record" is returned.
  *
  * if current namespace is "" and there is a binding of name "snet" to
  * "snet.feis.herts.ac.uk" and the element name is "snet:record", 
  * name "snet.feis.herts.ac.uk:record" is returned.
  *
  */
 static char *normalizeName(char *name){
   if(name == NULL){
     return name;
   }
   char *n = strchr(name, ':');
   if(n != NULL){
     char *ns = searchNS(name, n - name);
     if(ns != NULL){
       char *t = STRcat(ns, ":");
       char *c = STRcat(t, name);
       SNetMemFree(t);
       t = NULL;
       return c;
     }
   }else{
     if(getDefaultNS() != NULL){
       char *t = STRcat(getDefaultNS(), ":");
       char *c = STRcat(t, name);
       SNetMemFree(t);
       t = NULL;
       return c;
     }
   }
   return name;
}
  
 /* Pop the top element of the namespace stack */
 static void popNS(){
   nsStack_t *ns = parser.ns_stack;

   if(ns != NULL){
     if(ns->next != NULL){
       if(ns->deflt != ns->next->deflt){
	 SNetMemFree(ns->deflt);
	 while(ns->bound != ns->next->bound){
	   ns_t *temp = ns->bound->next;
	   deleteNS(ns->bound);
	   ns->bound = temp;
	 }
       }
     }else{
       SNetMemFree(ns->deflt);

       while(ns->bound != NULL){
	 ns_t *temp = ns->bound->next;
	 deleteNS(ns->bound);
	 ns->bound = temp;
       }
     }
     
     parser.ns_stack = ns->next;
     SNetMemFree(ns);
   }
 }

 /* Push new element on top of the namespace stack */
 static void pushNS(){
   nsStack_t *ns = SNetMemAlloc(sizeof(nsStack_t));
   
   if(parser.ns_stack != NULL){
     ns->deflt = parser.ns_stack->deflt;
   }else{
     ns->deflt = NULL;
   }
   ns->bound = parser.bound_ns;

   ns->next = parser.ns_stack;
   parser.ns_stack = ns;
 }

 /* Get the top most state in the state stack */
 static int getState(){
   if(parser.state_stack != NULL){
     return parser.state_stack->state;
   }
   else{
     return -1;
   }
 }

 /* Remove the top most state in the state stack */
 static int popState(){
   if(parser.state_stack != NULL){
     state_t *temp = parser.state_stack;
     parser.state_stack = parser.state_stack->next;
     int i = temp->state;
     SNetMemFree(temp);
     return i;
   }
   return -1;
 }

 /* Add the state as top most state in the state stack */
 static void pushState(int state){
   state_t *temp = SNetMemAlloc(sizeof(state_t));
   temp->state = state;
   temp->next = parser.state_stack;
   parser.state_stack = temp;
 }           

%}

%union {
  int           cint;
  char          *str;
  struct pair   *attrib;
}

%token TAG_END DQUOTE SQUOTE XMLDECL_BEGIN XMLDECL_END EQ STARTTAG_BEGIN STARTTAG_SHORTEND 
%token ENDTAG_BEGIN COMMENT_BEGIN COMMENT_END VERSION VERSIONNUMBER ENCODING STANDALONE YES NO

%token <str>  NAME CHARDATA DATTVAL SATTVAL COMMENTDATA ENCNAME 

%type <str> Content Contents 
%type <cint>  EndTag EmptyElemTag StartTag Element
%type <attrib> Attributes

%start Document

%%
Document:     Prolog Element 
              {
		switch($2)
		  {
		  case DATA:
		    YYACCEPT;
		    break;
		  default:
		    parser.terminate = PARSE_ERROR;
		    YYABORT;
		    break;

		  }           
              }
           ;

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

/* Empty. This rule only keep record of the namespace stack */
PushNS:    
           {
             pushNS();
           }


/* Use data of XML-elements. */
Element:      PushNS EmptyElemTag
              {
		popState();
		$$ = UNKNOWN;

		if($2 == DATA){
		  parser.current.mode = MODE_UNKNOWN;
		  $$ = DATA;
		  
		}else if($2 == RECORD_DATA){
		  // Records are added to the buffer (empty record)
		  SNetBufPut(parser.buffer, parser.current.record);
		  parser.current.record = NULL;
		  $$ = RECORD_DATA;
		}else if($2 == RECORD_SYNC){
		  $$ = RECORD_SYNC;
		}else if($2 == RECORD_COLLECT){
		  $$ = RECORD_COLLECT;
		}else if($2 == RECORD_SORT_BEGIN){
		  $$ = RECORD_SORT_BEGIN;
		}else if($2 == RECORD_SORT_END){
		  $$ = RECORD_SORT_END;
		}else if($2 == RECORD_TERMINATE){
		  $$ = RECORD_TERMINATE;

		}else if($2 == FIELD){
		  // Fields are added to the current record
		  // TODO: EMPTY FIELD -> error report instead?
		  int index = searchIndexByLabel(parser.labels, parser.current.label);
		  if(index != LABEL_ERROR && parser.current.interface != INTERFACE_NUM_UNKNOWN){
		    void *data = NULL;
		    
		    if(index < parser.labels->number_of_labels){
		      data = parser.func_deserialize(NULL);
		    }else{ // Data in unknown fields is stored as characters
		      data = (void *)STRcpy("");
		    }
		    
		    /*** This might change according to the interface */
		    C_Data *field = C2SNet_cdataCreate(data, parser.func_free, parser.func_copy);
		    /**************************************************/
		    
		    SNetRecAddField(parser.current.record, index);
		    SNetRecSetField(parser.current.record, index, (void *)field);

		    SNetRecSetInterfaceId(parser.current.record, parser.current.interface);
		  }
		  
		  if(parser.current.label != NULL){
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		  }
		  
		  parser.current.interface = INTERFACE_NUM_UNKNOWN;

		  $$ = FIELD;

		}else if($2 == TAG){
		  // Tags are added to the current record (tag with no given value!)
		  int index = searchIndexByLabel(parser.labels, parser.current.label);
		  if(index != LABEL_ERROR){
		    SNetRecAddTag(parser.current.record, index);
		    if(parser.current.label != NULL){
		      SNetMemFree(parser.current.label);
		      parser.current.label = NULL;
		    }
		  }
		  $$ = TAG;

		}else if($2 == BTAG){
		  // BTags are added to the current record (btag with no given value!)
		  int index = searchIndexByLabel(parser.labels, parser.current.label);
		  if(index != LABEL_ERROR){
		    SNetRecAddBTag(parser.current.record, index);
		    if(parser.current.label != NULL){
		      SNetMemFree(parser.current.label);
		      parser.current.label = NULL;
		    }
		  }
		  $$ = BTAG;
		}

		popNS();
              }

            | PushNS StartTag Content EndTag
              {
		$$ = UNKNOWN;
		if(($2 == $4) || ($4 == RECORD && (($2 == RECORD_DATA)       
						   || ($2 == RECORD_SYNC)      
						   || ($2 == RECORD_COLLECT)    
						   || ($2 == RECORD_SORT_BEGIN)
						   || ($2 == RECORD_SORT_END)
						   || ($2 == RECORD_TERMINATE)))) {	     
		  if($2 == DATA){
		    parser.current.mode = MODE_UNKNOWN;
		    $$ = DATA;

		  }else if($2 == RECORD_DATA){
		    // Records are added to the buffer
		    SNetBufPut(parser.buffer, parser.current.record);
		    parser.current.record = NULL;
		    $$ = RECORD_DATA;
		  }else if($2 == RECORD_SYNC){
		    $$ = RECORD_SYNC;
		  }else if($2 == RECORD_COLLECT){
		    $$ = RECORD_COLLECT;
		  }else if($2 == RECORD_SORT_BEGIN){
		    $$ = RECORD_SORT_BEGIN;
		  }else if($2 == RECORD_SORT_END){
		    $$ = RECORD_SORT_END;
		  }else if($2 == RECORD_TERMINATE){
		    $$ = RECORD_TERMINATE;

		  }else if($2 == FIELD){
		    // Fields are added to the current record
		    int index = searchIndexByLabel(parser.labels, parser.current.label);
		    if(index != LABEL_ERROR && parser.current.interface != INTERFACE_NUM_UNKNOWN){
		      void *data = NULL;

		      if(index < parser.labels->number_of_labels){
			data = parser.func_deserialize($3);
		      }else{ // Data in unknown fields is stored as characters
			data = (void *)STRcpy($3);
		      }

		      /*** This might change according to the interface */
		      C_Data *field = C2SNet_cdataCreate(data, parser.func_free, parser.func_copy);
		      /**************************************************/
		      
		      SNetRecAddField(parser.current.record, index);
		      SNetRecSetField(parser.current.record, index, (void *)field);

		      SNetRecSetInterfaceId(parser.current.record, parser.current.interface);
		    }
		    
		    if(parser.current.label != NULL){
		      SNetMemFree(parser.current.label);
		      parser.current.label = NULL;
		    }
		    
		    parser.current.interface = INTERFACE_NUM_UNKNOWN;
		    
		    $$ = FIELD;

		  }else if($2 == TAG){
		    // Tags are added to the current record
 		    int index = searchIndexByLabel(parser.labels, parser.current.label);
		    if(index != LABEL_ERROR){
		      SNetRecAddTag(parser.current.record, index);
		      
		      if($3 != NULL){
			SNetRecSetTag(parser.current.record, index, atoi($3));
		      }		      
		    }
		    if(parser.current.label != NULL){
		      SNetMemFree(parser.current.label);
		      parser.current.label = NULL;
		    }
		    
		    $$ = TAG;
		  }else if($2 == BTAG){
		    // BTags are added to the current record
 		    int index = searchIndexByLabel(parser.labels, parser.current.label);
		    if(index != LABEL_ERROR){
		      SNetRecAddBTag(parser.current.record, index);
		      
		      if($3 != NULL){
			SNetRecSetBTag(parser.current.record, index, atoi($3));
		      }
		    }

		    if(parser.current.label != NULL){
		      SNetMemFree(parser.current.label);
		      parser.current.label = NULL;
		    }
		    $$ = BTAG;
		  }
		}else{
		  yyerror("Unmatched XML-tags!");
		  printf("%d and  %d in state %d\n", $2, $4, getState());
		}

		if($3 != NULL){
		  SNetMemFree($3);
		}

		popState();
		popNS();
              }
            ;
  
/* Collect attributes and name of an ampty element tag. */
EmptyElemTag: STARTTAG_BEGIN NAME Attributes STARTTAG_SHORTEND
              {   
      	        char *name = normalizeName($2);
		if($2 != NULL && name != $2){
		  SNetMemFree($2);
		}

		$$ = UNKNOWN;

		//At data element only the mode is stored

		if(STRcmp(name, SNET_NS_DATA) == 0 
		   && getState() == ROOT){

		  pushState(DATA);

		  attrib_t *at = searchAttribute($3, MODE);

		  if(at != NULL){
		    if(STRcmp(at->value, TEXTUAL) == 0){
		      parser.current.mode = MODE_TEXTUAL;
		    }else if(STRcmp(at->value, BINARY) == 0){
		      parser.current.mode = MODE_BINARY;
		    }
		  }
		  $$ = DATA;

	        }//New record is created according to the type of the record:
	        else if(STRcmp(name, SNET_NS_RECORD) == 0 
			&& getState() == DATA){

		  attrib_t *at = searchAttribute($3, TYPE);

		  if(parser.current.record != NULL){
		    //TODO: This should be an error!
		    SNetRecDestroy(parser.current.record);
		    parser.current.record = NULL;
		    yyerror("Old record found!");
		  }

		  if(at != NULL){
		    if(strcmp(at->value, SNET_REC_DATA) == 0){
		      // New data record
		      pushState(RECORD_DATA);
		      parser.current.record = SNetRecCreate(REC_data, 
					            SNetTencVariantEncode( 
					              SNetTencCreateVector( 0), 
						      SNetTencCreateVector( 0), 
						      SNetTencCreateVector( 0)));
		      
		      $$ = RECORD_DATA;
		    }else if(strcmp(at->value, SNET_REC_SYNC) == 0){
		      pushState(RECORD_SYNC);
		      // TODO: What additional data is needed?
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_sync));
		      $$ = RECORD_SYNC;
		    }else if(strcmp(at->value, SNET_REC_COLLECT) == 0){
		      pushState(RECORD_COLLECT);
		      // TODO: What additional data is needed?
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_collect));
		      $$ = RECORD_COLLECT;
		    }else if(strcmp(at->value, SNET_REC_SORT_BEGIN) == 0){
		      pushState(RECORD_SORT_BEGIN);
		      // TODO: What additional data is needed?
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_sort_begin));
		      $$ = RECORD_SORT_BEGIN;
		    }else if(strcmp(at->value, SNET_REC_SORT_END) == 0){
		      pushState(RECORD_SORT_END);
		      // TODO: What additional data is needed?
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_sort_end));
		      $$ = RECORD_SORT_END;
		    }else if(strcmp(at->value, SNET_REC_TERMINATE) == 0){
		      // New control record: terminate
		      pushState(RECORD_TERMINATE);
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_terminate));
		      parser.terminate = PARSE_TERMINATE;
		      $$ = RECORD_TERMINATE;
		    }else{
		      yyerror("Record with unknown type found!");
		      // TODO: record with unknown type!
		    }
		  }
		  else{
		    yyerror("Record with no type found!");
		    // TODO: record with no type!
		  }

		}//Fields label is stored:
	        else if(STRcmp(name,SNET_NS_FIELD) == 0 
			&& getState() == RECORD_DATA){
		  
		  pushState(FIELD);

		  attrib_t *at = searchAttribute($3, LABEL);

		  if(parser.current.label != NULL){
		    //TODO: This should be an error!
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		  }
		  if(at != NULL){
		    parser.current.label = STRcpy(at->value);
		  }else{
		    //TODO: No label: This should be an error!
		    yyerror("Field with no label found!");
		  }

		  at = searchAttribute($3, INTERFACE);

		  if(at != NULL){

		    /*** This might change according to the interface */
		    if(STRcmp(at->value, INTERFACE_C2SNET) == 0){ 
		      parser.current.interface = INTERFACE_NUM_C2SNET;
		    /**************************************************/
		    }else{
		      //TODO: Unknown interface: This should be an error!
		      yyerror("Field with unknown interface found!");
		    }
		  }else{
		    //TODO: No interface: This should be an error!
		    yyerror("Field with no interface found!");
		    
		  }

		  $$ = FIELD;

	        }//Tags label is stored:
	        else if(STRcmp(name, SNET_NS_TAG) == 0 
			&& getState() == RECORD_DATA){

		  pushState(TAG);

		  attrib_t *at = searchAttribute($3, LABEL);

		  if(parser.current.label != NULL){
		    //TODO: This should be an error!
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		    yyerror("Old label found!");
		  }

		  if(at != NULL){
		    parser.current.label = STRcpy(at->value);
		  }else{
		    //TODO: No label: This should be an error!
		    yyerror("Tag with no label found!");
		  }

		  $$ = TAG;

	        }//BTags label is stored:
	        else if(STRcmp(name,SNET_NS_BTAG) == 0 
			&& getState() == RECORD_DATA){

		  pushState(BTAG);

		  attrib_t *at = searchAttribute($3, LABEL);

		  if(parser.current.label != NULL){
		    //TODO: This should be an error!
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		    yyerror("Old label found!");
		  }

		  if(at != NULL){
		    parser.current.label = STRcpy(at->value);
		  }else{
		    //TODO: No label: This should be an error!
		    yyerror("BTag with no label found!");
		  }

		  $$ = BTAG;
	        }else{
		  pushState(UNKNOWN);
		  $$ = UNKNOWN;
		}

		//Free memory
		if(name != NULL){
		  SNetMemFree(name);
		  name = NULL;
		}
		if($3 != NULL){
		  attrib_t *attr = $3;
		  while(attr != NULL){
		    attrib_t *temp = attr->next;
		    deleteAttribute(attr);
		    attr = temp;
		  }
		}
               }

            ;

/* Collect attributes and name of a start tag. */
StartTag:     STARTTAG_BEGIN NAME Attributes TAG_END
              {  
      	        char *name = normalizeName($2);
		if($2 != NULL && name != $2){
		  SNetMemFree($2);
		}
		//At data element only the mode is stored

                if(STRcmp(name, SNET_NS_DATA) == 0 
		   && getState() == ROOT){

		  pushState(DATA);

		  attrib_t *at = searchAttribute($3, MODE);

		  if(at != NULL){
		    if(STRcmp(at->value, TEXTUAL) == 0){
		      parser.current.mode = MODE_TEXTUAL;
		    }else if(STRcmp(at->value, BINARY) == 0){
		      parser.current.mode = MODE_BINARY;
		    }
		  }
		  $$ = DATA;

	        }//New record is created according to the type of the record: 
	        else if(STRcmp(name, SNET_NS_RECORD) == 0 
			&& getState() == DATA){

		  attrib_t *at = searchAttribute($3, TYPE);
		  
		  if(parser.current.record != NULL){
		    //TODO: This should be an error!
		    SNetRecDestroy(parser.current.record);
		    parser.current.record = NULL;
		    yyerror("Old record found!");
		  }

		  if(at != NULL){
		    if(strcmp(at->value, SNET_REC_DATA) == 0){
		      // New data record
		      pushState(RECORD_DATA);
		      parser.current.record = SNetRecCreate(REC_data, 
					            SNetTencVariantEncode( 
				                      SNetTencCreateVector( 0), 
						      SNetTencCreateVector( 0), 
						      SNetTencCreateVector( 0)));
		      
		      $$ = RECORD_DATA;
		    }else if(strcmp(at->value, SNET_REC_SYNC) == 0){
		      pushState(RECORD_SYNC);
		      // TODO: What additional data is needed?
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_sync));
		      $$ = RECORD_SYNC;
		    }else if(strcmp(at->value, SNET_REC_COLLECT) == 0){
		      pushState(RECORD_COLLECT);
		      // TODO: What additional data is needed?
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_collect));
		      $$ = RECORD_COLLECT;
		    }else if(strcmp(at->value, SNET_REC_SORT_BEGIN) == 0){
		      pushState(RECORD_SORT_BEGIN);
		      // TODO: What additional data is needed?
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_sort_begin));
		      $$ = RECORD_SORT_BEGIN;
		    }else if(strcmp(at->value, SNET_REC_SORT_END) == 0){
		      pushState(RECORD_SORT_END);
		      // TODO: What additional data is needed?
		      SNetBufPut(parser.buffer, SNetRecCreate(REC_sort_end));
		      $$ = RECORD_SORT_END;
		    }else if(strcmp(at->value, SNET_REC_TERMINATE) == 0){
		      // New control record: terminate
		      pushState(RECORD_TERMINATE);
		      SNetBufPut(parser.buffer, SNetRecCreate( REC_terminate));
		      parser.terminate = PARSE_TERMINATE;
		      $$ = RECORD_TERMINATE;
		    }else{
		      // TODO: record with unknown type!
		      yyerror("Record with unknown type found!");
		    }
		  }
		  else{
		    // TODO: record with no type!
		    yyerror("Record with no type found!");
		  }

	        }//Fields label is stored:
	        else if(STRcmp(name, SNET_NS_FIELD) == 0 
			&& getState() == RECORD_DATA){
		  pushState(FIELD);

		  attrib_t *at = searchAttribute($3, LABEL);

		  if(parser.current.label != NULL){
		    //TODO: This should be an error!
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		    yyerror("Old label found!");
		  }

		  if(at != NULL){
		    parser.current.label = STRcpy(at->value);
		  }else{
		    //TODO: No label: This should be an error!
		    yyerror("Field with no label found!");
		  }

		  at = searchAttribute($3, INTERFACE);

		  if(at != NULL){

		    /*** This might change according to the interface */
		    if(STRcmp(at->value, INTERFACE_C2SNET) == 0){ 
		      parser.current.interface = INTERFACE_NUM_C2SNET;
		    /**************************************************/
		    }else{
		      //TODO: Unknown interface: This should be an error!
		      yyerror("Field with unknown interface found!");
		    }
		  }else{
		    //TODO: No interface: This should be an error!
		    yyerror("Field with no interface found!");
		  }

		  $$ = FIELD;

	        }//Tags label is stored:
	        else if(STRcmp(name, SNET_NS_TAG) == 0 
			&& getState() == RECORD_DATA){

		  pushState(TAG);

		  attrib_t *at = searchAttribute($3, LABEL);

		  if(parser.current.label != NULL){
		    //TODO: This should be an error!
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		    yyerror("Old label found!");
		  }

		  if(at != NULL){
		    parser.current.label = STRcpy(at->value);
		  }else{
		    //TODO: No label: This should be an error!
		    yyerror("Tag with no label found!");
		  }

		  $$ = TAG;
	        }
	        else if(STRcmp(name, SNET_NS_BTAG) == 0 
			&& getState() == RECORD_DATA){
		  pushState(BTAG);

		  attrib_t *at = searchAttribute($3, LABEL);

		  if(parser.current.label != NULL){
		    //TODO: This should be an error!
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		    yyerror("Old label found!");
		  }

		  if(at != NULL){
		    parser.current.label = STRcpy(at->value);
		  }else{
		    //TODO: No label: This should be an error!
		    yyerror("BTag with no label found!");
		  }

		  $$ = BTAG;
	        }
		else{
		  pushState(UNKNOWN);
		  $$ = UNKNOWN;  
		}

		//Free memory
		if(name != NULL){
		  SNetMemFree(name);
		  name = NULL;
		}
		if($3 != NULL){
		  attrib_t *attr = $3;
		  while(attr != NULL){
		    attrib_t *temp = attr->next;
		    deleteAttribute(attr);
		    attr = temp;
		  }
		}
              }
            ;

/* Forms data structs from XML-attributes. 
 * Removes namespace attributes, but add them to namespace stack */
Attributes:   NAME EQ SQUOTE SATTVAL SQUOTE Attributes 
              {  
         
		//Default namespace
		if(STRcmp($1, "xmlns") == 0){
		  setDefaultNS($4);
		  SNetMemFree($1);
		  $$ = $6;
		}// Bound namespace 
		else if(strncmp($1, "xmlns:",6) == 0){
		  char *t = STRcpy($1 + 6);
		  initNS(t, $4);
		  SNetMemFree($1);
		  $$ = $6;
		}// Normal attribute
		else{
		  attrib_t *temp = initAttribute($1, $4);
		  temp->next = $6;
		  $$ = temp;
		}
              }

            | NAME EQ DQUOTE DATTVAL DQUOTE Attributes 
              {  
		//Default namespace
		if(STRcmp($1, "xmlns") == 0){
		  setDefaultNS($4);
		  SNetMemFree($1);
		  $$ = $6;
		}// Bound namespace  
		else if(strncmp($1, "xmlns:",6) == 0){
		  char *t = STRcpy($1 + 6);
		  initNS(t, $4);
		  SNetMemFree($1);
		  t = NULL;
		  $$ = $6;
		}// Normal attribute
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
            ;

/* Checks the tag value and returns it for matching start and end tags */
EndTag:       ENDTAG_BEGIN NAME TAG_END
              {  
		char *name = normalizeName($2);

		if(STRcmp(name, SNET_NS_DATA) == 0){
		  $$ = DATA;
		} 
		else if(STRcmp(name, SNET_NS_RECORD) == 0){
		  $$ = RECORD;
		}
		else if(STRcmp(name, SNET_NS_FIELD) == 0){
		  $$ = FIELD;
		}
		else if(STRcmp(name, SNET_NS_TAG) == 0){
		  $$ = TAG;
		}
		else if(STRcmp(name, SNET_NS_BTAG) == 0){
		  $$ = BTAG;
		}
		else{	        
		  $$ = UNKNOWN;  
		} 

		if(name != NULL && name != $2){
		  SNetMemFree(name);
		  name = NULL;
		}
		if($2 != NULL){
		  SNetMemFree($2);
		}
              }
            ;

/* At the moment the left most CHARDATA is returned */

Content:      CHARDATA Contents
              {               
                $$ =  $1;
              }

            | Contents
              {
                $$ = NULL;
              }
           ; 

/* At the moment the CHARDATA is just ignored if there is element inside the same content. */
Contents:    Element CHARDATA Contents
              {
		if($2 != NULL){
		  SNetMemFree($2);
		}
                $$ = NULL;
              }

            | Element Contents 
              {
                $$ = NULL;
              }
            | //EMPTY
              {
                $$ = NULL;
              }
            ;

%%

void yyerror(char *error) 
{
  printf("\nParse error: %s\n", error);
}


void parserInit(snet_buffer_t *in_buf,
		label_t *label,
		void *(*fdeserialize)(const char*),
	        void (*ffree)( void*),
		void* (*fcopy)( void*)){

  parser.buffer = in_buf;
  parser.labels = label;
  
  parser.func_deserialize = fdeserialize;
  parser.func_free = ffree;
  parser.func_copy = fcopy;

  parserFlush();
}  

void parserFlush(){

  parser.current.mode = MODE_UNKNOWN;

  if(parser.current.record != NULL){
    SNetMemFree(parser.current.record);
  }
  parser.current.record = NULL;
  

  if(parser.current.label != NULL){
    SNetMemFree(parser.current.label);
  }
  parser.current.label = NULL;
  
  
  parser.current.interface = INTERFACE_NUM_UNKNOWN;

  while(parser.bound_ns != NULL){
    ns_t *temp = parser.bound_ns->next;
    deleteNS(parser.bound_ns);
    parser.bound_ns = temp;
  }
  
  while(parser.ns_stack != NULL){
    popNS();
  }
  
  while(parser.state_stack != NULL){
    popState();
  }
  pushState(ROOT);
  
  parser.terminate = PARSE_CONTINUE;
}


int parserParse(){
  parserFlush();
  
  yyparse();

  return parser.terminate;
}

void parserDelete(){
  parserFlush();

  popState();
  
  parser.terminate = PARSE_TERMINATE;

  yylex_destroy();
}


#undef UNKNOWN           
#undef ROOT              
#undef DATA              
#undef RECORD            
#undef RECORD_DATA       
#undef RECORD_SYNC       
#undef RECORD_COLLECT    
#undef RECORD_SORT_BEGIN 
#undef RECORD_SORT_END   
#undef RECORD_TERMINATE  
#undef FIELD             
#undef TAG               
#undef BTAG              

#undef MODE_UNKNOWN 
#undef MODE_TEXTUAL 
#undef MODE_BINARY  

#undef SNET_NS_DATA 
#undef SNET_NS_RECORD  
#undef SNET_NS_FIELD   
#undef SNET_NS_TAG      
#undef SNET_NS_BTAG     

#undef SNET_REC_DATA 
#undef SNET_REC_SYNC 
#undef SNET_REC_COLLECT   
#undef SNET_REC_SORT_BEGIN 
#undef SNET_REC_SORT_END  
#undef SNET_REC_TERMINATE  

#undef LABEL
#undef INTERFACE
#undef MODE
#undef TEXTUAL
#undef BINARY
#undef TYPE

#undef INTERFACE_C2SNET

#undef INTERFACE__NUM_UNKNOWN 
#undef INTERFACE_NUM_C2SNET 
