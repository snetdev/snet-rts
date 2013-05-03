/*******************************************************************************
 *
 * $Id: parser.l 2864 2010-09-17 11:28:30Z dlp $
 *
 * Author: Juka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   14.12.2007
 * -----
 *
 * Description:
 * ------------
 *
 * Scanner of S-Net network interface
 *
 *******************************************************************************/

%{
#include <string.h>
#ifdef OLD_BUILD_SYSTEM
#include "parser.tab.h"
#else
#include "parser.h"
#endif

#define LINEBUF_SIZE 256
#define YY_NO_INPUT

#define STRcpy(text)    strdup(text)

#define FILTER(ret) return(ret);

#define YY_INPUT(buf,result,max_size) \
 { \
  int c = getc(yyin); \
  result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
 }

extern void yyerror(char *error);
%}
   /* set positions to 6000 */         
%p 6000
   /* set nodes to 1100 */
%e 1100 

%option noyywrap
%option stack
%option noyy_top_state

   /* Suppress compilation warning yyunput defined but not used */
%option nounput

  /* Lexer states */
 
    /* Inside XML-comment (<!-- ... -->) */
%x IN_XML_COMMENT
    /* Inside XML-tag (<...> or <.../>) */
%x IN_XML_ELEMENT 
    /* Inside XML-endtag (</...>) */
%x IN_XML_ENDELEMENT 
    /* Inside XML-declaration (<?xml ...?>) */
%x IN_XML_DECL  
    /* Inside XML-element's body */
%x IN_XML_CONTENT
    /* Inside double quoted XML-attribute ("...") */
%x IN_XML_DQUOTE 
    /* Inside single quoted XML-attribute ('...') */
%x IN_XML_SQUOTE 
    /* These are used to ignore all unknown elements */

%%

<*>{
 [ \t\n]+ {
         /* Ignore */
         
       }
}

<IN_XML_ELEMENT>{
 "/>" {
        
        yy_pop_state();
        FILTER( STARTTAG_SHORTEND);
      }
	
 "\"" {
        
        yy_push_state(IN_XML_DQUOTE);
        FILTER( DQUOTE);
      }

 "'"  {
        
        yy_push_state(IN_XML_SQUOTE);
        FILTER( SQUOTE);
      }

 ">"  {
        
        yy_pop_state();
        yy_push_state(IN_XML_CONTENT);
        FILTER( TAG_END);
      }

(([A-Za-z\200-\377_^?]|"_"|":")([A-Za-z\200-\377_0-9.-]|":")*) {
             
             yylval.str = STRcpy(yytext);
             FILTER( NAME);
      } 
}


<IN_XML_ENDELEMENT>{
">"  {
        
        yy_pop_state();
        FILTER( TAG_END);
     }
}                      
      
<IN_XML_CONTENT>{
 "</record" {
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          FILTER( RECORD_END_BEGIN);
        }

 "</field" {
          
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          FILTER( FIELD_END_BEGIN);
        }
 "</tag" {
          
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          FILTER( TAG_END_BEGIN);
        }
 "</btag" {
          
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          FILTER( BTAG_END_BEGIN);
        }
 
 "<field"    {
         
          yy_push_state(IN_XML_ELEMENT);
          FILTER( FIELD_BEGIN);
        }

 "<tag"    {
          
          yy_push_state(IN_XML_ELEMENT);
          FILTER( TAG_BEGIN);
        }

 "<btag" {
          
          yy_push_state(IN_XML_ELEMENT);
          FILTER( BTAG_BEGIN);
        }

 "<!--" {
          
          yy_push_state(IN_XML_COMMENT);
        }

 [^<>]+ { 
          
          yylval.str = STRcpy(yytext);
          FILTER( CHARDATA);
        }
}
 

<IN_XML_DQUOTE>{
 "\""     {
            
            yy_pop_state();
            FILTER( DQUOTE);
          }

 [^<&\"]* {
            
            yylval.str = STRcpy(yytext);
            FILTER( DATTVAL);
          }
}


<IN_XML_SQUOTE>{
 "'"     {
           
           yy_pop_state();
           FILTER( SQUOTE);
         }

 [^<&']* { 
           
           yylval.str = STRcpy(yytext);
           FILTER( SATTVAL);
         }
}


<IN_XML_DECL,IN_XML_ELEMENT>"=" {
                           
                           FILTER( EQ);
                         }


<IN_XML_DECL>{
 "version"                   {
                               
                               FILTER( VERSION);
                             }

 "encoding"                  {
                               
                               FILTER( ENCODING);
                             }

 "standalone"                {
                               
                               FILTER( STANDALONE);
                             }

 "yes"                       {
                               
                               FILTER( YES);
                             }

 "no"                        {
                               
                               FILTER( NO);
                             }

 "'"                         {
                               
                               FILTER( SQUOTE);
                             }

 "\""                        {
                               
                               FILTER( DQUOTE);
                             }

 "1.0"                       {
                               
                               FILTER( VERSIONNUMBER);
                             }

 [A-Za-z]([A-Za-z0-9._]|\-)* {   
                               
                               yylval.str = STRcpy(yytext);
                               FILTER( ENCNAME);
                             }

 "?>"                        {
                               
                               yy_pop_state();
                               FILTER( XMLDECL_END);
                             }
}


<IN_XML_COMMENT>{
 ([^-\n]|("-"[^-\n]))* {
                     /* Ignore */
                     
                   }
 
 "-->"             {
                     
                     yy_pop_state();
                   }
}


<INITIAL>{
 "<?xml" {
          yy_push_state(IN_XML_DECL);
          FILTER( XMLDECL_BEGIN);
        }

 "<!--" {    
           yy_push_state(IN_XML_COMMENT);
        }

 "<record" {
          yy_push_state(IN_XML_ELEMENT);
          FILTER( RECORD_BEGIN);
        }

 .      {
           /* Skip unknown characters until a record is found! */
        }
}

<*>.    { 
            yyerror("invalid character");           
        }
 
%%

/* This function is used to flush the lexer content and start lexin from the
   beginning. */

void yy_flush() 
{
  yyrestart(yyin);
  BEGIN(INITIAL);
}
