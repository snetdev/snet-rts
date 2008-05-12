/*******************************************************************************
 *
 * $Id: 
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
 * Main function for S-NET network interface.
 *
 *******************************************************************************/

#include <record.h>
#include <parser.h>
#include <output.h>
#include <observers.h>
#include "networkinterface.h"

int SNetInRun(char *static_labels[], int number_of_labels, 
	      char *static_interfaces[], int number_of_interfaces, 
	      snet_buffer_t *(snetfun)(snet_buffer_t *))
{
    int i = 0;
    snet_buffer_t *in_buf = SNetBufCreate(10);
    snet_buffer_t *out_buf = NULL;
    snetin_label_t *labels = NULL;
    snetin_interface_t *interfaces = NULL;

    labels     = SNetInLabelInit(static_labels, number_of_labels);
    interfaces = SNetInInterfaceInit(static_interfaces, number_of_interfaces);

    SNetObserverInit(labels, interfaces);

    out_buf = snetfun(in_buf);

    if(SNetInOutputInit(labels, interfaces, out_buf) != 0){
        return 1;
    }

    SNetInParserInit(labels, interfaces, in_buf);

    i = SNET_PARSE_CONTINUE;
    while(i != SNET_PARSE_TERMINATE){
        i = SNetInParserParse();
    }

    if(SNetInOutputDestroy() != 0){
        return 1;
    }

    if(in_buf != NULL){
        SNetBufDestroy(in_buf);
    }

    SNetInParserDestroy();

    SNetObserverDestroy();

    SNetInLabelDestroy(labels);
    SNetInInterfaceDestroy(interfaces);

    return 0;
}
