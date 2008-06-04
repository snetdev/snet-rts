/*******************************************************************************
 *
 * $Id$
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
#include "parser.tab.h"
#include <memfun.h>

#define LINEBUF_SIZE 256
#define YY_NO_UNPUT

char *STRcpy(const char *text){
  char *t = SNetMemAlloc(sizeof(char) * (strlen(text) + 1));
  return strcpy(t, text);
}

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
%x IN_XML_UNKNOWN
%x IN_XML_UNKNOWN_END
 
 
%%

<*>{
 [ \t\n]+ {
         /* Ignore */
       }
}

<IN_XML_ELEMENT>{
 "/>" {
        yy_pop_state();
        return STARTTAG_SHORTEND;
      }
	
 "\"" {
        BEGIN(IN_XML_DQUOTE);
        return DQUOTE;
      }

 "'"  {
        BEGIN(IN_XML_SQUOTE);
        return SQUOTE;
      }

 ">"  {
        BEGIN(IN_XML_CONTENT);
        return TAG_END;
      }
}


<IN_XML_ENDELEMENT>{
">"  {
        yy_pop_state();
        return TAG_END;
     }
}


<IN_XML_ELEMENT>(([A-Za-z\200-\377_^?]|"_"|":")([A-Za-z\200-\377_0-9.-]|":")*) {
             yylval.str = STRcpy(yytext);
             return NAME;
          }                           
      
<IN_XML_CONTENT>{ /* This is for error recovery! */
"<?xml" {
           while(yy_top_state() != INITIAL) {
              yy_pop_state();
           } 
        }

"</data" {
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          return DATA_END_BEGIN;
        }
"</record" {
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          return RECORD_END_BEGIN;
        }
"</field" {
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          return FIELD_END_BEGIN;
        }
"</tag" {
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          return TAG_END_BEGIN;
        }
"</btag" {
          yy_pop_state();
          yy_push_state(IN_XML_ENDELEMENT);
          return BTAG_END_BEGIN;
        }

 "<data"    {
          yy_push_state(IN_XML_ELEMENT);
          return DATA_BEGIN;
        }

 "<record"    {
          yy_push_state(IN_XML_ELEMENT);
          return RECORD_BEGIN;
        }

 "<field"    {
          yy_push_state(IN_XML_ELEMENT);
          return FIELD_BEGIN;
        }

 "<tag"    {
          yy_push_state(IN_XML_ELEMENT);
          return TAG_BEGIN;
        }

 "<btag" {
          yy_push_state(IN_XML_ELEMENT);
          return BTAG_BEGIN;
        }

 "<!--" {
          yy_push_state(IN_XML_COMMENT);
        }

 "<"[^ \n\t><]+ {
          yy_push_state(IN_XML_UNKNOWN);
        }

 [^<]*  { 
          yylval.str = STRcpy(yytext);
          return CHARDATA;
        }
}
 

<IN_XML_DQUOTE>{
 "\""     {
            BEGIN(IN_XML_ELEMENT);
            return DQUOTE;
          }

 [^<&\"]* {
            yylval.str = STRcpy(yytext);
            return DATTVAL;
          }
}


<IN_XML_SQUOTE>{
 "'"     {
           BEGIN(IN_XML_ELEMENT);
           return SQUOTE;
         }

 [^<&']* { 
           yylval.str = STRcpy(yytext);
           return SATTVAL;
         }
}


<IN_XML_DECL,IN_XML_ELEMENT>"=" {
                           return EQ;
                         }


<IN_XML_DECL>{
 "version"                   {
                               return VERSION;
                             }

 "encoding"                  {
                               return ENCODING;
                             }

 "standalone"                {
                               return STANDALONE;
                             }

 "yes"                       {
                               return YES;
                             }

 "no"                        {
                               return NO;
                             }

 "'"                         {
                               return SQUOTE;
                             }

 "\""                        {
                               return DQUOTE;
                             }

 "1.0"                       {
                               return VERSIONNUMBER;
                             }

 [A-Za-z]([A-Za-z0-9._]|\-)* {         
                               yylval.str = STRcpy(yytext);
                               return ENCNAME;
                             }

 "?>"                        {
                               BEGIN(INITIAL);
                               return XMLDECL_END;
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
          BEGIN(IN_XML_DECL);
          return XMLDECL_BEGIN;
        }

 "<!--" {
          yy_push_state(IN_XML_COMMENT);
        }


 "<data"    {
          yy_push_state(IN_XML_ELEMENT);
          return DATA_BEGIN;
        }

 "<record"    {
          yy_push_state(IN_XML_ELEMENT);
          return RECORD_BEGIN;
        }

 "<field"    {
          yy_push_state(IN_XML_ELEMENT);
          return FIELD_BEGIN;
        }

 "<tag"    {
          yy_push_state(IN_XML_ELEMENT);
          return TAG_BEGIN;
        }

 "<btag" {
          yy_push_state(IN_XML_ELEMENT);
          return BTAG_BEGIN;
        }

 "<"[^ \n\t><]+ {
          yy_push_state(IN_XML_UNKNOWN);
        }
}

<IN_XML_UNKNOWN_END>{
">" {
       yy_pop_state();
    }

 .  {
    }
}

<IN_XML_UNKNOWN>{
"<?xml" { /* This is for error recovery! */
           while(yy_top_state() != INITIAL) {
              yy_pop_state();
           } 
        }
 
 ">"  {

      }

 "</" {
          yy_pop_state();
          yy_push_state(IN_XML_UNKNOWN_END);
      }

 "<"  {
          yy_push_state(IN_XML_UNKNOWN);
      }

 "/>" {
          yy_pop_state();
      }

 .    {
      }
}

<*>.    {
            yyerror("invalid character");
        }

%%
