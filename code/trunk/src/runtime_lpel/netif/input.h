#ifndef _INPUT_H_
#define _INPUT_H_


#include <stdio.h>
#include "record.h"
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
    snetin_interface_t *interfaces,
    snet_stream_t *in_buf
    );

#endif /* _INPUT_H_ */
