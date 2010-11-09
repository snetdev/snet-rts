/*
 * Implements graphical boxes.
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <buffer.h>
#include <string.h>

extern snet_tl_stream_t*
SNetGraphicalBox( snet_tl_stream_t *instream, char **names, char* name);


extern snet_tl_stream_t*
SNetGraphicalFeeder(snet_tl_stream_t *outstream, char **names, int num_names );


extern void SNetInitGraphicalSystem();



#endif
