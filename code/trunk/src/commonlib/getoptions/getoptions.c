/*
 *
 * $Log$
 * Revision 3.4  2005/06/28 15:43:29  sah
 * added missing include
 *
 * Revision 3.3  2005/04/07 16:16:27  cg
 * Added string compare function with case sensitivity flag.
 *
 * Revision 3.2  2001/05/15 13:50:19  cg
 * Bug fixed in handling of option copy buffer.
 *
 * Revision 3.1  2000/11/20 18:02:05  sacbase
 * new release made
 *
 * Revision 2.3  1999/06/01 12:54:38  cg
 * added #include <string.h> for usage of function strncpy.
 *
 * Revision 2.2  1999/05/12 14:24:44  cg
 * old comment eliminated.
 *
 * Revision 2.1  1999/05/12 14:23:35  cg
 * new release made
 *
 * Revision 1.1  1999/05/12 13:49:44  cg
 * Initial revision
 *
 *
 */


/*****************************************************************************
 * 
 * file:    main_args.c
 *
 * prefix:        
 *
 * description:
 *
 *  This file contains the definitions of helper functions used by the macros
 *  defined in main_args.h for command line analysis.
 *   
 *****************************************************************************/


#include <stdio.h>   /* for NULL only */
#include <string.h>  /* for strncpy() */
#include <ctype.h>   /* for tolower() */

#define MAX_OPT_LEN 64
#define MIN(a,b) ((a)<(b)?(a):(b))


static char buffer[MAX_OPT_LEN];


/******************************************************************************
 *
 * function:
 *   int ARGS_CheckOption(char *pattern, char *argv1, char *argv2, 
 *                        char **option, char **argument)
 *
 * description:
 * 
 *  This function gets two consecutive entries from the command line, 
 *  argv1 and argv2. It compares the first argument, i.e. pattern, 
 *  against argv1. If these are equal, the function decides that argv2
 *  must be the argument to the option specified by pattern. If argv2
 *  does not start with '-', everything is fine and argv1 is returned as
 *  option and argv2 as argument. The actual return value is 2, indicating
 *  that two command line entries have been processed.
 *
 *  If pattern is a prefix of argv1, then the remaining characters are assumed
 *  to represent the argument; the return parameters argument and option are
 *  set accordingly. The actual return value is 1, indicating
 *  that one command line entry has been processed.
 *
 *  If pattern is not even a prefix of argv1, then the actual return value 
 *  is 0, indicating that none of the command line entries have been processed.
 *
 ******************************************************************************/

int ARGS_CheckOption(char *pattern, char *argv1, char *argv2, 
                     char **option, char **argument)
{
  int i=0;
  int res=1;
  
  if (argv1[0] != '-') 
  {
    *option = NULL;
    *argument = NULL;
    return(0);
  }
    
  while (pattern[i] != '\0') 
  {
      /*
       * The function is finished if <pattern> is not a prefix of
       * the current <argv>.
       */
    if (pattern[i] != argv1[i+1])
    {
      *option = NULL;
      *argument = NULL;
      return(0);
    }
    
    i++;
  }
  
  if (argv1[i+1] == '\0') 
  {
    /*
     * <pattern> and the current <argv> are identical.
     */

    *option = argv1;
    
    if (argv2 == NULL)
    {
      *argument = NULL;
    }
    else 
    {
      if (argv2[0] == '-') 
      {
        *argument = NULL;
      }
      else 
      {
        *argument = argv2;
        res = 2;
      }
    }
  }
  else 
  {
    int len = MIN(i+1, MAX_OPT_LEN-1);
    
    strncpy( buffer, argv1, len);
    buffer[len] = '\0';
    /*
     * The library function strncpy() itself does NOT append
     * a terminating 0 character.
     */

    *option = buffer;
    *argument = argv1 + i + 1;
  }
  
  return(res);
}




/******************************************************************************
 *
 * function:
 *   bool ARGS_StringCompare( char *s1, char *s2, int case_sensitive)
 *
 * description:
 * 
 *  This function compares two strings s1 and s2 for equality. The third
 *  (boolean) parameter specifies case sensitivity.
 *
 ******************************************************************************/

int ARGS_StringEqual( char *s1, char *s2, int case_sensitive)
{
  int i;
  
  if ((s1==NULL) && (s2==NULL)) {
    return( 1);
  }
  if  ((s1==NULL) || (s2==NULL)) {
    return( 0);
  }

  if (case_sensitive) {
    for (i=0; (s1[i]!='\0') && (s2[i]!='\0'); i++) {
      if (s1[i] != s2[i]) {
        return(0);
      }
    }
  }
  else {
    for (i=0; (s1[i]!='\0') && (s2[i]!='\0'); i++) {
      if (tolower(s1[i]) != tolower(s2[i])) {
        return(0);
      }
    }
  }

  if ((s1[i]=='\0') && (s2[i]=='\0')) {
    return(1);
  }
  else {
    return(0);
  }
}

  
