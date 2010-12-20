#ifndef _INPUT_H_
#define _INPUT_H_


#include <stdio.h>
#include "record.h"
#include "stream.h"
#include "label.h"
#include "interface.h"


/* Init input system.
 *
 * @param file File where the input is read from.
 * @param labels Set of labels to use.
 * @param interfaces Set of interfaces to use.
 * @param in_buf Buffer to where the records are put.
 *
 */
void SNetInInputInit(FILE *file,
    snetin_label_t *labels, 
#ifdef DISTRIBUTED_SNET
    snetin_interface_t *interfaces
#else /* DISTRIBUTED_SNET */
    snetin_interface_t *interfaces,
    stream_t *in_buf
#endif/* DISTRIBUTED_SNET */
    );
           



#endif /* _INPUT_H_ */
