
#ifndef CONSTANTS_H
#define CONSTANTS_H

// define initial size of buffers
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 250
#endif


// define initial size of repository for
// buffers in SNetSplit()
//#ifndef INITIAL_REPOSITORY_SIZE
//#define INITIAL_REPOSITORY_SIZE 10
//#endif

// Used for record entries
#ifndef UNSET
#define UNSET -1
#endif
#ifndef CONSUMED
#define CONSUMED -2
#endif
#ifndef NOT_FOUND
#define NOT_FOUND -3
#endif


// Used for graphic boxes
#ifndef MAX_BOXTITLE_LEN
#define MAX_BOXTITLE_LEN 255
#endif
#ifndef MAX_INT_LEN
#define MAX_INT_LEN 15
#endif
#ifndef PORT_RANGE_START
#define PORT_RANGE_START 6555 
#endif
#ifndef FEEDER_PORT
#define FEEDER_PORT 6999
#endif


#endif

