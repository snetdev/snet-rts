/*
 * moninfo.c
 *
 *  Created on: 29. July  2011
 *      Author: Raimund Kirner
 *
 * This file includes data types and functions to maintain monitoring information.
 * The main purpose at creation (July 2011) was to have monitoring information for
 * each S-Net record to log the overall execution time in a (sub)network from reading
 * a record until the output of all of its descendant records at the exit of this
 * (sub)network.
 *
 */
#include <assert.h>

#include "moninfo.h"
#include "memfun.h"
#include "debug.h"
#include "string.h"



/**
 * CAUTION!
 * Keep following names consistent with header file
 */

static const char names_event[] = {
		//  "EV_INPUT_ARRIVE",
		//  "EV_BOX_START",
		//  "EV_BOX_WRITE",
		//  "EV_BOX_FINISH",
		//  "EV_FILTER_START",
		//  "EV_FILTER_WRITE",
		//  "EV_SYNC_FIRST",
		//  "EV_SYNC_NEXT",
		//  "EV_SYNC_FIRE",
		'I',
		'O',
};



/**
 * Initialize a moninfo for a record descriptor
 *
 * Handled differently for different events
 * @pre MONINFOPTR(mon) is already allocated
 */
void MonInfoInitRec(snet_moninfo_t *mon, va_list args)
{
	assert( MONINFO_DESCR( mon) == MON_RECORD );

	snet_record_t *rec = va_arg( args, snet_record_t *);

	switch ( REC_DESCR( rec)) {
	case REC_data: /* currently only data records can be monitored this way */

		/* initialize fields */
		SNetRecIdGet( &REC_MONINFO( mon, id), rec);
		REC_MONINFO( mon, add_moninfo_rec_data) = NULL; /*FIXME*/

		switch ( MONINFO_EVENT(mon) ) {
		//        case EV_INPUT_ARRIVE:
		//          break;
		//        case EV_BOX_START:
		//        case EV_FILTER_START:
		//        case EV_SYNC_FIRST:
		//        case EV_SYNC_NEXT:
		//          break;
		//        case EV_BOX_WRITE:
		//        case EV_FILTER_WRITE:
		//          break;
		//        case EV_SYNC_FIRE:
		//          break;
		//        case EV_BOX_FINISH:
		//          break;
		case EV_MESSAGE_IN:
		case EV_MESSAGE_OUT:
			break;
		default:
			SNetUtilDebugFatal("Unknown monitoring information event. [event=%d]", MONINFO_EVENT(mon));
		} /* MONINFO_EVENT( mon) */
		break;
		default:
			SNetUtilDebugFatal("Non-supported monitoring information record."
					"[event=%d][record-descr=%d]", MONINFO_EVENT(mon), REC_DESCR( rec));
	} /* REC_DESCR( rec) */

}

/*****************************************************************************
 * Create monitoring information (entries depend on monitoring item)
 ****************************************************************************/
snet_moninfo_t *SNetMonInfoCreate ( snet_moninfo_event_t event, snet_moninfo_descr_t descr,... )
{
	snet_moninfo_t *mon;
	va_list args;

	mon = SNetMemAlloc( sizeof( snet_moninfo_t));
	MONINFO_DESCR( mon) = descr;
	MONINFO_EVENT( mon) = event;

	va_start( args, descr);
	switch (descr) {
	case MON_RECORD:
		MONINFOPTR( mon) = SNetMemAlloc( sizeof( snet_moninfo_record_t));
		/* call function to initialize record monitoring specific data */
		MonInfoInitRec(mon, args);
		break;
	default:
		SNetUtilDebugFatal("Unknown monitoring information description. [%d]", descr);
		break;
	}
	va_end( args);
	return mon;
}


/*****************************************************************************
 * Delete monitoring information (entries depend on monitoring item)
 ****************************************************************************/
void SNetMonInfoDestroy( snet_moninfo_t *mon)
{
	switch (MONINFO_DESCR( mon)) {
	case MON_RECORD:
		if (REC_MONINFO( mon, add_moninfo_rec_data) != NULL) {
			SNetMemFree( REC_MONINFO( mon, add_moninfo_rec_data));
		}
		SNetMemFree( MONINFOPTR( mon));
		break;
	default:
		SNetUtilDebugFatal("Unknown monitoring information description. [%d]", MONINFO_DESCR( mon));
		break;
	}
	SNetMemFree(mon);
}



/*****************************************************************************
 * Create a copy of the additional data of record monitoring information
 ****************************************************************************/
snet_add_moninfo_rec_data_t SNetMonInfoRecCopyAdditionalData( snet_add_moninfo_rec_data_t add_data)
{
	snet_add_moninfo_rec_data_t new_add_data;
	if (add_data != NULL) {
		new_add_data = (snet_add_moninfo_rec_data_t) strdup ( add_data);
		if (new_add_data == NULL)
			SNetUtilDebugFatal("Copy of additional monitoring information for records failed. [%s]", add_data);
	}
	else {
		new_add_data = NULL;
	}
	return new_add_data;
}

static void PrintMonInfoId( FILE *f, snet_record_id_t *id)
{
	fprintf(f, "%u.%u",
			id->subid[1], id->subid[0]
	);
}

void SNetMonInfoPrint( FILE *f, snet_moninfo_t *mon)
{
	fprintf(f,
			"%c",
			names_event[MONINFO_EVENT(mon)]
	);


	switch (mon->mon_descr) {
	case MON_RECORD: /* monitoring of a record */
		PrintMonInfoId( f, &(REC_MONINFO( mon, id)) );
		break;
	default: /*ignore*/
		break;
	}

}

int SNetMonRecTypeData(void *item) {
	snet_record_t *rec = (snet_record_t *)item;
  return (REC_DESCR( rec) == REC_data);
}
