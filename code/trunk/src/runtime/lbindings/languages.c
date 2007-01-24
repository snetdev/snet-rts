/*
 * box language integration
 *   functions to use "copy" and "free"
 *   conversion from snet-int to boxlanguage-int
 */

#include <stdio.h>
#include <languages.h>


extern void *SNetCopyField( snet_record_t *rec, int name) {
  
  void *res;

  switch( SNetRecGetLanguage( rec)) {
    case snet_lang_sac:
      /* insert SAC specific code here */ 
      break;
    
    default:
      printf("\n\n ** Fatal Error ** : Language integration missing [%d]\n\n",
             SNetRecGetLanguage( rec));
  }

  return( res);
}


extern void SNetFreeField( snet_record_t *rec, int name) {
  
  switch( SNetRecGetLanguage( rec)) {
    case snet_lang_sac:
      /* insert SAC specific code here */ 
      break;
    
    default:
      printf("\n\n ** Fatal Error ** : Language integration missing [%d]\n\n",
             SNetRecGetLanguage( rec));
  }
}





