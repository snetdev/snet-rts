/*
 * fetcher.h
 *
 *  Created on: May 22, 2014
 *      Author: thiennga
 */

#ifndef FETCHER_H_
#define FETCHER_H_

#include "stream.h"

typedef struct {
  snet_stream_t *input;
} fetch_arg_t;

void SNetFetcher(snet_stream_t *input);

#endif /* FETCHER_H_ */
