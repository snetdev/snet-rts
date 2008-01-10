/*** THIS NEEDS TO BE GENERATED (directly to factorial.h ?) ***/
// TODO: Better names for global variables!

#include <myfuns.h>

#define NUMBER_OF_LABELS 6

const char *const snet_static_labels[NUMBER_OF_LABELS] = {"F_none"         , "F__factorial__x", 
							  "F__factorial__p", "T__factorial__T",
							  "T__factorial__F", "T__factorial__stop"};
#define NUMBER_OF_INTERFACES 1

#define INTERFACE_C2SNET     0
                                                  
const char *const snet_language_interfaces[NUMBER_OF_INTERFACES] = {"C2SNet"};

char *(* const snet_serialization_funcs[NUMBER_OF_INTERFACES])(const void *)  = {myserialize};
void *(* const snet_deserialization_funcs[NUMBER_OF_INTERFACES])(const char*) = {mydeserialize};


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
#include "output.h"

#include "factorial.h"
#include <C2SNet.h>

int main(int argc, char* argv[])
{
  /** initialize **/

  int inBufSize = 10;
  label_t *labels = initLabels(snet_static_labels, NUMBER_OF_LABELS);
  interface_t *interfaces = initInterfaces(snet_language_interfaces, snet_serialization_funcs, 
					   snet_deserialization_funcs, NUMBER_OF_INTERFACES);
  snet_buffer_t *in_buf = SNetBufCreate(inBufSize);
  snet_buffer_t *out_buf = NULL;

  SNetGlobalInitialise();

  /*** THIS NEEDS TO BE GENERATED */
  C2SNet_init(INTERFACE_C2SNET);

  out_buf = SNet__factorial___factorial(in_buf);
  /********************************/
  
  initOutput(labels, interfaces);

  if(startOutput(out_buf) != 0){
    return 1;
  }

  parserInit(in_buf, labels, interfaces);

  /** action loop **/

  int i = PARSE_CONTINUE;
  while(i != PARSE_TERMINATE) {
    i = parserParse();
  }  

  /** destroy **/

  if(in_buf != NULL){
    SNetBufBlockUntilEmpty(in_buf);
    SNetBufDestroy(in_buf);
  }
  
  parserDelete();

  if(blockUntilEndOfOutput() != 0){
    return 1;
  }

  deleteLabels(labels);
  deleteInterfaces(interfaces);         
  
  return 0;
}
