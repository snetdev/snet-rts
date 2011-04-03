#ifndef PATTERN_H
#define PATTERN_H

typedef struct snet_pattern snet_pattern_t;

snet_pattern_t *SNetPatternCreate( snet_int_list_t *tags, snet_int_list_t *btags, snet_int_list_t *fields);
void SNetPatternDestroy( snet_pattern_t *pat);

#endif
