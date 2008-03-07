/*** THIS NEEDS TO BE GENERATED (directly to factorial.h ?) ***/
// TODO: Better names for global variables!

#include "myfuns.h"

#define SNET__factorial__NUMBER_OF_INTERFACES 1

#define I__factorial__C2SNET 0
                                                  
char *snet_factorial_interfaces[SNET__factorial__NUMBER_OF_INTERFACES] = {"C2SNet"};


/**************************************************************/


/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   10.1.2008
 * -----
 *
 * Description:
 * ------------
 *
 * main function for S-NET network interface.
 *
 *******************************************************************************/

/* Current problems and TODO:
 *
 * 1) How the extra names should be deleted?
 *    - Ref counting
 * 2) How and where this code should be generated?
 *    - Compiler seems to be obvious choice
 * 3) Error conditions?
 *    - parser.y, main.c
 * 4) Other control records than terminate?
 *    - What additional data is needed? (or are these needed at all?)
 * 5) Serialization/deserialization functions?
 *    - Are they ok? Should some other data be added (like field name)?
 */

#include <buffer.h>
#include <record.h>
#include "parser.h"
#include "label.h"
#include "interface.h"
#include "globals.h"
#include "output.h"

#include <graphics.h>
#include "factorial.h"
#include <C2SNet.h>
#include <stdio.h>

snetin_label_t *globals_labels = NULL;
snetin_interface_t *globals_interfaces = NULL;

int main(int argc, char* argv[])
{
  /** initialize **/

  int inBufSize = 10;

  /*** THIS NEEDS TO BE GENERATED */                              
  globals_labels = SNetInLabelInit(snet_factorial_labels, SNET__factorial__NUMBER_OF_LABELS);
  globals_interfaces = SNetInInterfaceInit(snet_factorial_interfaces, 
						       SNET__factorial__NUMBER_OF_INTERFACES);
  /********************************/

  snet_buffer_t *in_buf = SNetBufCreate(inBufSize);
  snet_buffer_t *out_buf = NULL;

  SNetGlobalInitialise();
  SNetInitGraphicalSystem();

  /*** THIS NEEDS TO BE GENERATED */
  C2SNet_init(I__factorial__C2SNET, mydeserialize);

  out_buf = SNet__factorial___factorial(in_buf);
  /********************************/
  
  SNetInOutputInit();

  if(SNetInOutputBegin(out_buf) != 0){
    return 1;
  }

  SNetInParserInit(in_buf);

  /** action loop **/

  int i = PARSE_CONTINUE;
  while(i != PARSE_TERMINATE) {
    i = SNetInParserParse();
   }  

  /** destroy **/

  if(in_buf != NULL){
    SNetBufBlockUntilEmpty(in_buf);
    SNetBufDestroy(in_buf);
  }
  
  SNetInParserDestroy();

  if(SNetInOutputBlockUntilEnd() != 0){
    return 1;
  }
  
  SNetInLabelDestroy(globals_labels);
  SNetInInterfaceDestroy(globals_interfaces);         
  
  return 0;
}
