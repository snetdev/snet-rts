/*
 * Implements graphical boxes.
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <buffer.h>
#include <string.h>

extern snet_buffer_t *SNetGraphicalBox( snet_buffer_t *inbuf, char **names, char* name);


extern snet_buffer_t *SNetGraphicalFeeder( snet_buffer_t *outbuf, char **names, int num_names );


extern void SNetInitGraphicalSystem();



#endif
