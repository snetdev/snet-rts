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

/* TODO: 
 * 1) Error reporting (most conditions commented to code with TODOs)
 * 2) Handling of malformed XML
 *    - If there is a missing end tag in some XML-element, the parsing of the data will never stop!
 * 3) Binary data mode is not working.
 *    - Should there be another deserialization function for binary data?
 */
%{

#include <stdio.h>
#include <stdlib.h>
#include <memfun.h>
#include <snetentities.h>
#include <parser.h>
#include <str.h>
#include <interface.h>
#include <globals.h>

/* Values to define state of the parser (in which kind of XML element) */
#define STACK_ERROR       -2
#define ERROR             -1
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

 extern int yylex(void);
 extern void yylex_destroy();
 void yyerror(char *error);



 /***** Struct definitions ******/
 
 /* Struct of two strings for a lists */
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

/* Struct for record queue for parsed records
 * (Parsed records cannot be put to inbuffer before the whole document is parsed) */

 typedef struct record_queue{
   snet_record_t *rec;
   struct record_queue * next;
 }rq_t;


 /***** Globals variables *****/
 struct{

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
  
   /* All the bound namespaces */
   ns_t *bound_ns; 

   /* Namespace stack for the parser */
   nsStack_t *ns_stack;
 
   /* State stack for the parser */
   state_t *state_stack;

   /* A Queue for sstoring records */
   rq_t *first;
   rq_t *last;

 }parser;




 /***** Auxiliary functions *****/
 

 /** Record queue functions **
  *
  *  These are used to store records temporarily before
  *  the whole XML-document is parsed. This is to prevent
  *  malformed XML-documents to be parsed to SNet.
  */

 /* Get next record from record queue. */
 snet_record_t *getRec(){
   rq_t *fst = parser.first;
   snet_record_t *temp = NULL;

   if(parser.first != NULL){
     temp = parser.first->rec;
     parser.first = parser.first->next;
     SNetMemFree(fst);
   }

   if(parser.first == NULL){
     parser.last = NULL;
   }
   
   return temp;
 }
 
 /* Put a record to record queue. */
 void putRec(snet_record_t *rec){
   rq_t *temp = SNetMemAlloc(sizeof(rq_t));
   temp->rec = rec;
   temp->next = NULL;

   if(parser.last != NULL){
     parser.last->next = temp;
     parser.last = temp;
   }else{
     parser.first = temp;
     parser.last = temp;
   }
 }

 /** Namespace functions **
  *
  * These functions are used to store namespaces
  * between parser states.
  *
  */

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
       if(len == strlen(ns->name)){
	 if(strncmp(ns->name, prefix, len) == 0){
	   return ns->value;
	 }
       }
       ns = ns->next;
     }
   }
   return NULL;
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
       char *c = STRcat(t, n + 1);
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
  
 /** State functions **
  *
  * These functions are used to change parser states.
  *
  */


 /* Get the top most state in the state stack */
 static int getState(){
   if(parser.state_stack != NULL){
     return parser.state_stack->state;
   }
   else{
     return STACK_ERROR;
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
   return STACK_ERROR;
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

%type <str> Content Contents StartTag EndTag
%type <cint>  EmptyElemTag Element
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

		  if(parser.terminate != PARSE_ERROR){
		    snet_record_t *temp = NULL;
		    while((temp = getRec()) != NULL){
		      SNetBufPut(parser.buffer, temp);
		    }
		  }

		  $$ = DATA;
		  
		}else if($2 == RECORD){ 
		  $$ = RECORD;
		}else if($2 == RECORD_DATA){
		  // Records are added to the buffer (empty record)
		  putRec(parser.current.record);
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
		  int index = SNetInSearchIndexByLabel(globals_labels, parser.current.label);
		  if(index != LABEL_ERROR && parser.current.interface != INTERFACE_UNKNOWN){
		    
		    if(parser.current.record != NULL){
		      if(SNetRecGetInterfaceId(parser.current.record) == INTERFACE_UNKNOWN){
			SNetRecSetInterfaceId(parser.current.record, parser.current.interface);
		      }
		      
		      if(SNetRecGetInterfaceId(parser.current.record) == parser.current.interface){

			void *field = NULL;

			if(parser.current.mode == MODE_TEXTUAL){
			  void *(*desfun)(char *, int) = SNetGetDeserializationFun(parser.current.interface);			
			  if(desfun != NULL) {
			    field = desfun(NULL, 0);
			  }else {
			    yyerror("Deserialization error!");
			  }

			}else if(parser.current.mode == MODE_BINARY){
			  yyerror("Deserialization of data in binary mode is not yet implemented!");
			  //TODO: Does binary mode actually need separate function at all?
			  void *(*desfun)(char *, int) = SNetGetDeserializationFun(parser.current.interface);		
			  if(desfun != NULL) {
			    field = desfun(NULL, 0);
			  }else {
			    yyerror("Deserialization error!");
			  }

			}else{
			  yyerror("Trying to deserialize data in unknown mode!");
			}
			if(field != NULL) {
			  SNetRecAddField(parser.current.record, index);
			  SNetRecSetField(parser.current.record, index, field);
			}else{
			  yyerror("Deserialization error: Field data == NULL!");
			}
		      }else{
			yyerror("Record has fields with multiple interfaces!");
			SNetRecDestroy(parser.current.record);
			parser.current.record = NULL;
		      }
		    }else{
		      yyerror("No record to add the field!");	      
		    }
		  }
		  if(parser.current.label != NULL){
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		  }
		  
		  parser.current.interface = INTERFACE_UNKNOWN;

		  $$ = FIELD;

		}else if($2 == TAG){
		  // Tags are added to the current record (tag with no given value!)
		  int index = SNetInSearchIndexByLabel(globals_labels, parser.current.label);
		  if(index != LABEL_ERROR){
		    if(parser.current.record != NULL){
		      SNetRecAddTag(parser.current.record, index);
		      if(parser.current.label != NULL){
			SNetMemFree(parser.current.label);
			parser.current.label = NULL;
		      }
		    }else{
		      yyerror("No record to add the tag!");	      
		    }
		  }
		  $$ = TAG;

		}else if($2 == BTAG){
		  // BTags are added to the current record (btag with no given value!)
		  int index = SNetInSearchIndexByLabel(globals_labels, parser.current.label);
		  if(index != LABEL_ERROR){
		    if(parser.current.record != NULL){
		      SNetRecAddBTag(parser.current.record, index);
		      if(parser.current.label != NULL){
			SNetMemFree(parser.current.label);
			parser.current.label = NULL;
		      }
		    }else{
		      yyerror("No record to add the btag!");	      
		    }
		  }
		  $$ = BTAG;
		}

		popNS();
              }

            | PushNS StartTag Content EndTag
              {
		$$ = UNKNOWN;

		if(strcmp($2, $4) == 0){
		  if(getState() == DATA){
		    parser.current.mode = MODE_UNKNOWN;

		    if(parser.terminate != PARSE_ERROR){
		      snet_record_t *temp = NULL;
		      while((temp = getRec()) != NULL){
			SNetBufPut(parser.buffer, temp);
		      }
		    }

		    $$ = DATA;
		  }else if(getState() == RECORD){
		    $$ = RECORD;
		  }else if(getState() == RECORD_DATA){
		    // Records are added to the buffer
		    putRec(parser.current.record);
		    parser.current.record = NULL;
		    $$ = RECORD_DATA;
		  }else if(getState() == RECORD_SYNC){
		    $$ = RECORD_SYNC;
		  }else if(getState() == RECORD_COLLECT){
		    $$ = RECORD_COLLECT;
		  }else if(getState() == RECORD_SORT_BEGIN){
		    $$ = RECORD_SORT_BEGIN;
		  }else if(getState() == RECORD_SORT_END){
		    $$ = RECORD_SORT_END;
		  }else if(getState() == RECORD_TERMINATE){
		    $$ = RECORD_TERMINATE;
		  }else if(getState() == FIELD){
		    // Fields are added to the current record
		    int index = SNetInSearchIndexByLabel(globals_labels, parser.current.label);
		    if(index != LABEL_ERROR && parser.current.interface != INTERFACE_UNKNOWN){

		      if(parser.current.record != NULL){
			if(SNetRecGetInterfaceId(parser.current.record) == INTERFACE_UNKNOWN){
			  SNetRecSetInterfaceId(parser.current.record, parser.current.interface);
			}

			if(SNetRecGetInterfaceId(parser.current.record) == parser.current.interface){

			  void *field = NULL;
			  if(parser.current.mode == MODE_TEXTUAL){
			    void *(*desfun)(char *, int) = SNetGetDeserializationFun(parser.current.interface);
			    if(desfun != NULL && $3 != NULL) {
			      field = desfun($3, strlen($3));
			    }else if($3 == NULL) {
			      field = desfun($3, 0);
			    }else {
			      yyerror("Deserialization error!");
			    }

			  }else if(parser.current.mode == MODE_BINARY){
			    yyerror("Deserialization of data in binary mode is not yet implemented!");
			    //TODO: Does binary mode actually need separate function at all?

			    void *(*desfun)(char *, int) = SNetGetDeserializationFun(parser.current.interface);			   
			    if(desfun != NULL && $3 != NULL) {
			      field = desfun($3, strlen($3));
			    }else if($3 == NULL) {
			      field = desfun($3, 0);
			    }else {
			      yyerror("Deserialization error!");
			    }

			  }else{
			    yyerror("Trying to deserialize data in unknown mode!");
			  }

			  if(field != NULL) {
			    SNetRecAddField(parser.current.record, index);
			    SNetRecSetField(parser.current.record, index, field);
			  }else {
			    yyerror("Deserialization error: Field data == NULL!");
			  }
			}else{
			  yyerror("Record has fields with multiple interfaces!");
			  SNetRecDestroy(parser.current.record);
			  parser.current.record = NULL;
			}
		      }else{
			yyerror("No record to add the field!");	      
		      }
		    }
		    
		    if(parser.current.label != NULL){
		      SNetMemFree(parser.current.label);
		      parser.current.label = NULL;
		    }
		    
		    parser.current.interface = INTERFACE_UNKNOWN;
		    
		    $$ = FIELD;

		  }else if(getState() == TAG){
		    // Tags are added to the current record
 		    int index = SNetInSearchIndexByLabel(globals_labels, parser.current.label);
		    if(index != LABEL_ERROR){
		      if(parser.current.record != NULL){
			SNetRecAddTag(parser.current.record, index);			
			if($3 != NULL){
			  SNetRecSetTag(parser.current.record, index, atoi($3));
			}	
		      }else{
			yyerror("No record to add the tag!");	      
		      }
		    }
		    if(parser.current.label != NULL){
		      SNetMemFree(parser.current.label);
		      parser.current.label = NULL;
		    }
		    
		    $$ = TAG;
		  }else if(getState() == BTAG){
		    // BTags are added to the current record
 		    int index = SNetInSearchIndexByLabel(globals_labels, parser.current.label);
		    if(index != LABEL_ERROR){
		      if(parser.current.record != NULL){
			SNetRecAddBTag(parser.current.record, index);
			
			if($3 != NULL){
			  SNetRecSetBTag(parser.current.record, index, atoi($3));
			}
		      }else{
			yyerror("No record to add the btag!");	      
		      }
		    }
		      
		    if(parser.current.label != NULL){
		      SNetMemFree(parser.current.label);
		      parser.current.label = NULL;
		    }
		    $$ = BTAG;
		  }
		}else{ 
		  //Ignored data (Unknown element or known element under wrong scope).
		  parser.terminate = PARSE_ERROR;
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

		if(strcmp(name, SNET_NS_DATA) == 0 && getState() == ROOT){


		    /* These should not matter as the data element is empty
		     * and will be ignored!
		    attrib_t *at = searchAttribute($3, MODE);
		    
		    if(parser.current.mode != MODE_UNKNOWN){
		      yyerror("Old mode found!");
		    }
		    
		    if(at != NULL){
		      if(strcmp(at->value, TEXTUAL) == 0){
			parser.current.mode = MODE_TEXTUAL;
		      }else if(strcmp(at->value, BINARY) == 0){
			parser.current.mode = MODE_BINARY;
		      }else{
			parser.current.mode = MODE_UNKNOWN;
			yyerror("Data element with unknown mode found!");
		      }
		    }else{
		      yyerror("Data element with no mode found!");
		    }
		    */

		  pushState(DATA);
		  $$ = DATA;

	        }//New record is created according to the type of the record:
	        else if(strcmp(name, SNET_NS_RECORD) == 0 && getState() == DATA){
		  
		  attrib_t *at = searchAttribute($3, TYPE);
		  
		  if(parser.current.record != NULL){
		    //TODO: This should be an error!
		    SNetRecDestroy(parser.current.record);
		    parser.current.record = NULL;
		    yyerror("Old record found!");
		  }
		  
		  if(at != NULL){
		    if(strcmp(at->value, SNET_REC_DATA) == 0){
		      // New (empty) data record
		      pushState(RECORD_DATA);
		      parser.current.record = SNetRecCreate(REC_data,
						  SNetTencVariantEncode( 
						      SNetTencCreateVector( 0), 
						      SNetTencCreateVector( 0), 
						      SNetTencCreateVector( 0)));

		      SNetRecSetInterfaceId(parser.current.record, INTERFACE_UNKNOWN);
		      $$ = RECORD_DATA;
		    }else if(strcmp(at->value, SNET_REC_SYNC) == 0){
		      pushState(RECORD_SYNC);
		      // TODO: What additional data is needed?
		      putRec(SNetRecCreate(REC_sync));
		      $$ = RECORD_SYNC;
		    }else if(strcmp(at->value, SNET_REC_COLLECT) == 0){
		      pushState(RECORD_COLLECT);
		      // TODO: What additional data is needed?
		      putRec(SNetRecCreate(REC_collect));
		      $$ = RECORD_COLLECT;
		    }else if(strcmp(at->value, SNET_REC_SORT_BEGIN) == 0){
		      pushState(RECORD_SORT_BEGIN);
		      // TODO: What additional data is needed?
		      putRec(SNetRecCreate(REC_sort_begin));
		      $$ = RECORD_SORT_BEGIN;
		    }else if(strcmp(at->value, SNET_REC_SORT_END) == 0){
		      pushState(RECORD_SORT_END);
		      // TODO: What additional data is needed?
		      putRec(SNetRecCreate(REC_sort_end));
		      $$ = RECORD_SORT_END;
		    }else if(strcmp(at->value, SNET_REC_TERMINATE) == 0){
		      // New control record: terminate
		      pushState(RECORD_TERMINATE);
		      putRec(SNetRecCreate(REC_terminate));
		      if(parser.terminate != PARSE_ERROR){
			parser.terminate = PARSE_TERMINATE;
		      }
		      $$ = RECORD_TERMINATE;
		    }else{
		      yyerror("Record with unknown type found!");
		      pushState(RECORD);
		      $$ = RECORD;
		      // TODO: record with unknown type!
		    }
		  }
		  else{
		    yyerror("Record with no type found!");
		    pushState(RECORD);
		    $$ = RECORD;
		      // TODO: record with no type!
		  }
		  
		}//Fields label is stored:
	        else if(strcmp(name,SNET_NS_FIELD) == 0 && getState() == RECORD_DATA){
		  
		  attrib_t *at = searchAttribute($3, LABEL);
		  
		  if(parser.current.label != NULL){
		    //TODO: This should be an error!
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		    yyerror("Old label found!");
		  }
		  
		  if(parser.current.interface != INTERFACE_UNKNOWN){
		    //TODO: This should be an error!
		    parser.current.interface = INTERFACE_UNKNOWN;
		    yyerror("Old interface found!");
		  }
		  
		  if(at != NULL){
		    parser.current.label = STRcpy(at->value);
		  }else{
		    //TODO: No label: This should be an error!
		    yyerror("Field with no label found!");
		    }
		  
		  at = searchAttribute($3, INTERFACE);
		  
		  if(at != NULL){
		    
		    int interface = SNetInInterfaceToId(globals_interfaces, at->value);
		    if(interface != INTERFACE_UNKNOWN){ 
		      parser.current.interface = interface;
		    }else{
		      //TODO: Unknown interface: This should be an error!
		      yyerror("Field with unknown interface found!");
		    }
		  }else{
		    //TODO: No interface: This should be an error!
		    yyerror("Field with no interface found!");
		    
		  }
		  
		  pushState(FIELD);
		  $$ = FIELD;
		  
	        }//Tags label is stored:
	        else if(strcmp(name, SNET_NS_TAG) == 0 && getState() == RECORD_DATA){

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

		  pushState(TAG);
		  $$ = TAG;

	        }//BTags label is stored:
	        else if(strcmp(name,SNET_NS_BTAG) == 0 && getState() == RECORD_DATA){
		  
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

		  pushState(BTAG);
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
		$$ = name;
		if($2 != NULL && name != $2){
		  SNetMemFree($2);
		}
		//At data element only the mode is stored

                if(strcmp(name, SNET_NS_DATA) == 0 && getState() == ROOT){

		  attrib_t *at = searchAttribute($3, MODE);
		  
		  if(parser.current.mode != MODE_UNKNOWN){
		    yyerror("Old mode found!");
		  }
		  
		  if(at != NULL){
		    if(strcmp(at->value, TEXTUAL) == 0){
		      parser.current.mode = MODE_TEXTUAL;
		    }else if(strcmp(at->value, BINARY) == 0){
		      parser.current.mode = MODE_BINARY;
		    }else{
		      parser.current.mode = MODE_UNKNOWN;
		      yyerror("Data element with unknown mode found!");
		    }
		  }else{
		    yyerror("Data element with no mode found!");
		  }
		  
		  pushState(DATA);
		  
	        }//New record is created according to the type of the record: 
	        else if(strcmp(name, SNET_NS_RECORD) == 0 && getState() == DATA){
		  
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
		      SNetRecSetInterfaceId(parser.current.record, INTERFACE_UNKNOWN);	      
		    }else if(strcmp(at->value, SNET_REC_SYNC) == 0){
		      pushState(RECORD_SYNC);
		      // TODO: What additional data is needed?
		      putRec(SNetRecCreate(REC_sync));
		    }else if(strcmp(at->value, SNET_REC_COLLECT) == 0){
		      pushState(RECORD_COLLECT);
		      // TODO: What additional data is needed?
		      putRec(SNetRecCreate(REC_collect));
		    }else if(strcmp(at->value, SNET_REC_SORT_BEGIN) == 0){
		      pushState(RECORD_SORT_BEGIN);
		      // TODO: What additional data is needed?
		      putRec(SNetRecCreate(REC_sort_begin));
		    }else if(strcmp(at->value, SNET_REC_SORT_END) == 0){
		      pushState(RECORD_SORT_END);
		      // TODO: What additional data is needed?
		      putRec(SNetRecCreate(REC_sort_end));
		    }else if(strcmp(at->value, SNET_REC_TERMINATE) == 0){
		      // New control record: terminate
		      pushState(RECORD_TERMINATE);
		      putRec(SNetRecCreate(REC_terminate));
		      if(parser.terminate != PARSE_ERROR){
			parser.terminate = PARSE_TERMINATE;
		      }
		    }else{
		      // TODO: record with unknown type!
		      pushState(RECORD);
		      yyerror("Record with unknown type found!");
		    }
		  }
		  else{
		    // TODO: record with no type!
		    pushState(RECORD);
		    yyerror("Record with no type found!");
		  }
		  
	        }//Fields label is stored:
	        else if(strcmp(name, SNET_NS_FIELD) == 0 && getState() == RECORD_DATA){
		  attrib_t *at = searchAttribute($3, LABEL);
		  
		  if(parser.current.label != NULL){
		    //TODO: This should be an error!
		    SNetMemFree(parser.current.label);
		    parser.current.label = NULL;
		    yyerror("Old label found!");
		  }
		  if(parser.current.interface != INTERFACE_UNKNOWN){
		    //TODO: This should be an error!
		    parser.current.interface = INTERFACE_UNKNOWN;
		    yyerror("Old interface found!");
		  }
		  
		  if(at != NULL){
		    parser.current.label = STRcpy(at->value);
		  }else{
		    //TODO: No label: This should be an error!
		    yyerror("Field with no label found!");
		  }
		  
		  at = searchAttribute($3, INTERFACE);
		  
		  if(at != NULL){
		    
		    int interface = SNetInInterfaceToId(globals_interfaces, at->value);
		    if(interface != INTERFACE_UNKNOWN){ 
		      parser.current.interface = interface;
		    }else{
		      //TODO: Unknown interface: This should be an error!
		      yyerror("Field with unknown interface found!");
		    }
		  }else{
		    //TODO: No interface: This should be an error!
		    yyerror("Field with no interface found!");
		  }

		  pushState(FIELD);
		  
	        }//Tags label is stored:
	        else if(strcmp(name, SNET_NS_TAG) == 0 && getState() == RECORD_DATA){
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

		  pushState(TAG);
	        }
	        else if(strcmp(name, SNET_NS_BTAG) == 0 && getState() == RECORD_DATA){
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
		  
		  pushState(BTAG);
	        }
		else{
		  pushState(UNKNOWN);
		}

		//Free memory

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
		if(strcmp($1, "xmlns") == 0){
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
		if(strcmp($1, "xmlns") == 0){
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
               $$ = normalizeName($2);

	       if($2 != NULL && $2 != $$){
		 SNetMemFree($2);
	       }
              }
            ;

/* At the moment the left most (i.e the first) CHARDATA is returned */

Content:      CHARDATA Contents
              {        
		if($2 != NULL){
		  SNetMemFree($2);
		}
                $$ =  $1;
              }

            | Contents
              {
                $$ = $1;
              }
           ; 

/* As explained beforem only the first CHARDATA is used. */
Contents:    Element CHARDATA Contents
              {
		if($3 != NULL){
		  SNetMemFree($3);
		}
                $$ = $2;
              }

            | Element Contents 
              {
                $$ = $2;
              }
            | /* EMPTY*/
              {
                $$ = NULL;
              }
            ;

%%

void yyerror(char *error) 
{
  printf("\nParse error: %s\n", error);
}

static void flush(){

  parser.current.mode = MODE_UNKNOWN;

  if(parser.current.record != NULL){
    SNetMemFree(parser.current.record);
  }
  parser.current.record = NULL;
  

  if(parser.current.label != NULL){
    SNetMemFree(parser.current.label);
  }
  parser.current.label = NULL;
  
  
  parser.current.interface = INTERFACE_UNKNOWN;

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

  snet_record_t *temp = NULL;
  while((temp = getRec()) != NULL){
    SNetRecDestroy(temp);
  }
  
  parser.first = NULL;
  parser.last = NULL;
  
  parser.terminate = PARSE_CONTINUE;
}

void SNetInParserInit(snet_buffer_t *in_buf){
  
  parser.buffer = in_buf;
  
  flush();
}  

int SNetInParserParse(){
  flush();
  
  yyparse();

  int temp = parser.terminate;

  flush();

  return temp;
}

void SNetInParserDestroy(){
  flush();

  popState();
  
  parser.terminate = PARSE_TERMINATE;

  yylex_destroy();
}

#undef STACK_ERROR
#undef ERROR
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
