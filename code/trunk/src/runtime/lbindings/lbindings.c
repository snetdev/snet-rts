/*
 * box language integration
 *   functions to use "copy" and "free"
 *   conversion from snet-int to boxlanguage-int
 */

#include <stdio.h>
#include <stdlib.h>

#include <bool.h>
//#include <lbindings.h>
#include <languages.h>


/* SAC includes */

// #include <cinterface.h>




extern void *SNetCopyField( void *field, snet_lang_descr_t lang) {
  
  void *res;

  if( field != NULL) {
    switch( lang) {
      case snet_lang_sac:
//        if( SAC_isValid( (SAC_arg*)field) == true) {
//          res = field;
//          SAC_IncRC( (SAC_arg*)res, 1);  
//        }
//        else {
//          printf("\n\n ** Fatal Error ** : SAC argument invalid\n\n");
//          exit( 1);
//        }
        break;
    
      default:
        printf("\n\n ** Fatal Error ** : Language integration missing [%d]\n\n",
               lang);
        exit( 1);
    }
  }
  else {
    res = NULL;
  }

  return( res);
}


extern void SNetFreeField( void *field, snet_lang_descr_t lang) {
 
  if( field != NULL) {
    switch( lang) {
      case snet_lang_sac:
//        if( SAC_isValid( (SAC_arg*)field) == true) {
//          SAC_DecRC( (SAC_arg*)field, 1);  
//        }
//        else {
//          printf("\n\n ** Fatal Error ** : SAC argument invalid\n\n");
//          exit( 1);
//        }
  
        break;
    
      default:
        printf("\n\n ** Fatal Error ** : Language integration missing [%d]\n\n",
               lang);
        exit( 1);
    }
  }
}


extern void *SNetTag2Int( int tag, snet_lang_descr_t lang) {

  void *res;

  switch( lang) {
    case snet_lang_sac:
//        res = SAC_Int2Sac( tag);  
      break;
    
    default:
      printf("\n\n ** Fatal Error ** : Language integration missing [%d]\n\n",
             lang);
      exit( 1);
  }

  return( res);
}

extern int SNetInt2Tag( void *val, snet_lang_descr_t lang) {

  int res;

  switch( lang) {
    case snet_lang_sac:
//        res = SAC_Sac2Int( val);  
      break;
    
    default:
      printf("\n\n ** Fatal Error ** : Language integration missing [%d]\n\n",
             lang);
      exit( 1);
  }

  return( res);
}
  
