/*
 * moninfo.h
 *
 *  Created on: Jul 29, 2011
 *      Author: Raimund Kirner
 * This file includes data types and functions to maintain monitoring information.
 * The main purpose at creation (July 2011) was to have monitoring information for
 * each S-Net record to log the overall execution time in a (sub)network from reading
 * a record until the output of all of its descendant records at the exit of this
 * (sub)network.
 *
 */

#ifndef MONINFO_H_
#define MONINFO_H_

typedef int record_id_t[3];  /* fields: 0..id (unique for thread), 1..thread_id, 2..node_id */




#endif /* MONINFO_H_ */
