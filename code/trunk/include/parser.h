#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include "snetentities.h"
#include "label.h"
#include "interface.h"

/* Return values of parserParse() */

#define SNET_PARSE_CONTINUE 0  /* Parsing can continue after this. */
#define SNET_PARSE_TERMINATE 1 /* Parsing should terminate after this. */
#define SNET_PARSE_ERROR 2     /* Error while parsing the last input data. */

/* Initialize the parse before parsing.
 *
 * @param file File where the input is read from.
 * @param labels Set of labels to use.
 * @param interfaces Set of interfaces to use.
 * @param in_buf Buffer where the parsed records are put.
 *
 * @notice parserInit should be the first call to the parser!
 */

extern void SNetInParserInit(FILE *file,
			     snetin_label_t *labels,
			     snetin_interface_t *interfaces,
			     snet_tl_stream_t *in_buf);


/* Parse the next data element from standard input stream 
 *
 * @return SNET_PARSE_CONTINUE Parsing can continue after this.
 * @return SNET_PARSE_TERMINATE Parsing should terminate after this.
 * @return SNET_PARSE_ERROR Error while parsing the last input data.
 *
 * @notice parserInit() MUST be called before the first call to parserParse()! 
 */

extern int SNetInParserParse();


/* Delete the parser and all data stored by it.
 * 
 * @notice Calling any parser functions after parserDelete() call might lead 
 *         to unexpected results!
 */

extern void SNetInParserDestroy();

#endif /* PARSER_H_ */
