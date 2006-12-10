
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#include <graphics.h>
#include <record.h>
#include <typeencode.h>
#include <buffer.h>
#include <memfun.h>
#include <bool.h>
#include <constants.h>


#define PRINT_RECORD_CONTENTS( RECNUM, RECNAMES, DESCR, DATA)\
    strcat(text, " - ");\
    itoa( RECNUM( rec), num_to_str, 10);\
    strcat( text, num_to_str);\
    strcat( text, DESCR);\
    rec_names = RECNAMES( rec);\
    for( i=0; i<RECNUM( rec); i++) {\
      strcat( text, names[ rec_names[i] ]);\
      strcat( text, "=");\
      itoa( DATA, num_to_str, 10);\
      strcat( text, num_to_str);\
      strcat( text, "  ");\
    }\
    strcat(text, "\n"); 



pthread_mutex_t *lock_port_counter;
int port_counter = PORT_RANGE_START;


typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf;
  char *box_title;
  char **descriptor;
  int port;
  int socket;
  int num_desc;
} graphic_handle_t;

static void itoa (int val, char *str, int rdx) {

  int i;
  int len = 0;
  int offset = 0;
  char result[MAX_INT_LEN + 1];

  result[MAX_INT_LEN] = '\0';
  offset = 48;                  /* 0=(char)48 */

  for( i = 0; i < MAX_INT_LEN; i++) {
      result[MAX_INT_LEN - 1 - i] = (char) ( ( val % rdx) + offset);
      val /= rdx;
      if ( val == 0) {
          len = ( i + 1);
          break;
      }
  }
  strcpy( str, &result[MAX_INT_LEN - len]);
}


static char *RecordToText( snet_record_t *rec, char *text, char **names) {
  int i;
  char num_to_str[MAX_INT_LEN + 1];
  int *rec_names;

  strcpy( text,  "The current record contains:\n\n");

  PRINT_RECORD_CONTENTS( SNetRecGetNumFields, SNetRecGetFieldNames, " fields : ", 
                *((int*)SNetRecGetField( rec, rec_names[i])));

  PRINT_RECORD_CONTENTS( SNetRecGetNumTags, SNetRecGetTagNames, " tags   : ", 
                SNetRecGetTag( rec, rec_names[i]));

  PRINT_RECORD_CONTENTS( SNetRecGetNumBTags, SNetRecGetBTagNames, " btags  : ", 
                SNetRecGetBTag( rec, rec_names[i]));
//  strcat( text, "\n");  
  return( text);
} 




static void *GraphicBoxThread( void *hndl) {

  graphic_handle_t *hnd = (graphic_handle_t*)hndl; 
  int pid = 0;
  char buf[4096];
  char port_no[10];
  struct hostent *host;
  struct sockaddr_in addr;
  int s;
  snet_buffer_msg_t buf_msg;
  snet_record_t *rec;
  int state;
  bool terminate = false;

  #define STATE_OK            0
  #define STATE_PUT_FAILED    1
  #define STATE_GET_FAILED    2
  

  /* fork the process */
  if( ( pid = fork()) < 0) {
    perror( "Cannot fork son");
  }


  /* I am the forked process ... */
  if( pid == 0) {
    itoa( hnd->port, port_no, 10);

    execl("RecordInspector", "SNET-RecordInspector", port_no, (char *)0);

    perror("\n\n ** Fatal Error ** : Could not start visualiser (not in $PATH?))\n\n");
    exit( 1);
  }

  // --------------------------

  sleep( 1);
  if (!inet_aton("localhost", &addr.sin_addr)) {
    host = gethostbyname("localhost");
    if (!host) {
      perror("gethostbyname() failed");
      exit( 1);
    }
    addr.sin_addr = *(struct in_addr*)host->h_addr;
  }

  s = socket(PF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror("socket() failed");
    exit( 1);
  }

  addr.sin_port = htons( hnd->port);
  addr.sin_family = AF_INET;

  if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("Graphical: connect() failed");
    exit( 1);
  }

  hnd->socket = s;
  // ---------------



  // --------------------------


  send(s, hnd->box_title, strlen( hnd->box_title) + 1, 0);

  state = STATE_GET_FAILED;

  while( !( terminate)) {
    
    recv(s, buf, 1024, 0);

    switch( state) {

      case STATE_OK:
      case STATE_PUT_FAILED: {
        SNetBufTryPut( hnd->outbuf, rec, &buf_msg);
        if( buf_msg != BUF_success) {
          strcpy( buf, "Buffer full.\n");
          send(s, buf, strlen( buf) + 1, 0);
          state = STATE_PUT_FAILED;
          break;
        }
        else {
          strcpy( buf, "\nRecord put to out buffer\nWaiting for next record to arrive\n");
          state = STATE_OK;
        }
      }
      case STATE_GET_FAILED: {
        rec = SNetBufTryGet( hnd->inbuf, &buf_msg);
        if( buf_msg == BUF_success) {
          switch( SNetRecGetDescriptor( rec)) {
            case REC_data:
              RecordToText( rec, buf, hnd->descriptor);
              send(s, buf, strlen( buf) + 1, 0);
              state = STATE_OK;
              break;
            case REC_terminate:
              strcpy( buf, "The current record contains:\n\n");
              strcat( buf, " - \n - control message: terminate\n - \n");
              terminate = true;
              send(s, buf, strlen( buf) + 1, 0);
              recv(s, buf, 1024, 0);
              SNetBufPut( hnd->outbuf, rec);
              break;
            case REC_sync:
              hnd->inbuf = SNetRecGetBuffer( rec);
              SNetRecDestroy( rec);
              break;
            default:
              strcpy( buf, "\nReceived unhandled control record!\n");
              terminate = true;
              send(s, buf, strlen( buf) + 1, 0);
              recv(s, buf, 1024, 0);
              break;
          }
          break;
        }
        else {
          strcpy( buf, "Buffer empty.\n");
          send(s, buf, strlen( buf) + 1, 0);
          state = STATE_GET_FAILED;
          break;
        }
       }
    }
 }
  close( s);
  return( NULL);
}












extern snet_buffer_t *SNetGraphicalBox( snet_buffer_t *inbuf, int inc_num, char **names, char *name) {

  snet_buffer_t *outbuf;
  pthread_t *box_thread;
  graphic_handle_t *hnd;

  int my_port;
  // -------------------



  // ------------------


  if( strlen( name) >= MAX_BOXTITLE_LEN) {
    printf("\n ** Fatal Error ** : Title for Box [%s] too long"
           " (max. %d characters)\n\n", name, MAX_BOXTITLE_LEN);
    exit( 1);
  }
  
  hnd = SNetMemAlloc( sizeof( graphic_handle_t));
  hnd->box_title = SNetMemAlloc( ( strlen( name) + 1) * sizeof( char));
  strcpy( hnd->box_title, name);
  hnd->descriptor = names;
  
  pthread_mutex_lock( lock_port_counter);
  my_port = port_counter++;
  pthread_mutex_unlock( lock_port_counter);
  hnd->port = my_port;
  
  outbuf = SNetBufCreate( BUFFER_SIZE);

  hnd->inbuf = inbuf;
  hnd->outbuf = outbuf;

  box_thread = SNetMemAlloc( sizeof( pthread_t));
  pthread_create( box_thread, NULL, GraphicBoxThread, (void*)hnd);
  pthread_detach( *box_thread);

  return( outbuf);
}

// ----------------------------------------------------------------------------



/*
 * The created list has the format:
 * num_entries, e1, e2, ..., e_num_entries,.
 * ",." forms the end of the string.
 */
static void NamesToString( char** names, int num, char *dest) {

  int i;
  char tmp[10];

  itoa( num, tmp, 10);

  strcpy( dest, tmp);
  strcat( dest, ",");

  for( i=0; i<num; i++) {
    strcat( dest, names[i]);
    strcat( dest, ",");
  }
}


static int GetNextName( char *src, int *i) {

  int j=0;
  char str[10];

  while( src[*i] != '=') {
//    printf("GetNextName: %c\n", src[*i]);
    str[j] = src[*i];
    *i += 1; j += 1;
  }
  str[j] = '\0';
//  printf("GetName: str: %s, val: %d\n", str, atoi( str));

  return( atoi( str));
}


static int GetNextValue( char *src, int *i) {

  int j=0;
  char str[10];

  *i += 1; // i points to '=' after GetNextName()

  while( src[*i] != ',') {
//    printf("GetNextValue: %c\n", src[*i]);

    str[j] = src[*i];
    *i += 1; j += 1;
  }
  str[j] = '\0';
//  printf("GetVal: str: %s, val: %d\n", str, atoi( str));
  return( atoi( str));
}

static void *ToVoidPtr( int val) {

  int *i;
//  printf("toPtr: val = %d\n", val);
  i = SNetMemAlloc( sizeof( int));
  *i = val;

  return( (void*)i);
}


#define IS_FIELD 0
#define IS_TAG   1
#define IS_BTAG  2
static int GetTypeOfName( int pos, char **names) {
  switch( names[pos][0]) {
    case 'F': return( IS_FIELD);
    case 'T': return( IS_TAG);
    case 'B': return( IS_BTAG);
    default: {
      printf("\n\n ** Fatal Error ** : Could not determine type of name.\n\n");
      exit( 1);
    }
  }
}

static snet_record_t *StringToRecord( char *new_rec_descr, char **names) {

  snet_record_t *rec;
  int i;
  snet_variantencoding_t *venc;
  int name;
  
  venc = SNetTencVariantEncode( 
          SNetTencCreateVector( 0), 
          SNetTencCreateVector( 0),
          SNetTencCreateVector( 0));

  rec = SNetRecCreate( REC_data, venc);

  i = 0; 
  while( new_rec_descr[i] != '\0') {
    name = GetNextName( new_rec_descr, &i);
    switch( GetTypeOfName( name, names)) {
      
      case IS_FIELD: {
        SNetRecAddField( rec, name);
        SNetRecSetField( rec, name, ToVoidPtr( GetNextValue( new_rec_descr, &i)));
        break;
      }
      case IS_TAG: {
        SNetRecAddTag( rec, name);
        SNetRecSetTag( rec, name, GetNextValue( new_rec_descr, &i));
        break;
      }
      case IS_BTAG: {
        SNetRecAddBTag( rec, name);
        SNetRecSetBTag( rec, name, GetNextValue( new_rec_descr, &i));
        break;
      }
      default: {
        /* Can never be reached, GetTypeOfName() exits first. */
      }
    }
//    printf("INCI %d",i);
    i += 1; // i points to ',' after GetNextValue()
//    printf("  %d\n",i);
  }

  return( rec);
}


static void *FeederThread( void *hndl) {


  graphic_handle_t *hnd = (graphic_handle_t*)hndl; 
  int pid = 0;
  char buf[4096];
  char port_no[10];
  struct hostent *host;
  struct sockaddr_in addr;
  int s;
  bool terminate = false;
  snet_record_t *rec;
  

  /* fork the process */
  if( ( pid = fork()) < 0) {
    perror( "Cannot fork son");
  }


  /* I am the forked process ... */
  if( pid == 0) {
    itoa( FEEDER_PORT, port_no, 10);

    execl("FeederFront", "SNET-Feeder", port_no, (char *)0);

    perror("\n\n ** Fatal Error ** : Could not start visualiser (not in $PATH?))\n\n");
    exit( 1);
  }
  /* ************************* */
    
  
  sleep( 1);
  if (!inet_aton("localhost", &addr.sin_addr)) {
    host = gethostbyname("localhost");
    if (!host) {
      perror("gethostbyname() failed");
      exit( 1);
    }
    addr.sin_addr = *(struct in_addr*)host->h_addr;
  }

  s = socket(PF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror("socket() failed");
    exit( 1);
  }

  addr.sin_port = htons( FEEDER_PORT);
  addr.sin_family = AF_INET;

  if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("Feeder: connect() failed");
    exit( 1);
  }

  hnd->socket = s;
  // ---------------



  // --------------------------

  // send description to graphical frontend
  NamesToString( hnd->descriptor, hnd->num_desc, buf);
  send(s, buf, strlen( buf) + 1, 0);  

  while( !( terminate)) {
    recv(s, buf, 4096, 0);
    
    if( strcmp( buf, "TERMINATE") == 0) {
      SNetBufPut( hnd->outbuf, SNetRecCreate( REC_terminate));
      terminate = true;
    } 
    else {
      rec = StringToRecord( buf, hnd->descriptor);
    }
    
    SNetBufPut( hnd->outbuf, rec);
  }

  close( s);
  return( NULL);
}




extern snet_buffer_t *SNetGraphicalFeeder( snet_buffer_t *outbuf, char **names, int num_names ) {

  graphic_handle_t *hnd;
  pthread_t *box_thread;

  hnd = SNetMemAlloc( sizeof( graphic_handle_t));
  hnd->outbuf = outbuf;
  hnd->descriptor = names;
  hnd->num_desc = num_names; 
  

  box_thread = SNetMemAlloc( sizeof( pthread_t));
  pthread_create( box_thread, NULL, FeederThread, (void*)hnd);
  pthread_detach( *box_thread);

  return( outbuf);

}


extern void SNetInitGraphicalSystem() {

  lock_port_counter = SNetMemAlloc( sizeof( pthread_mutex_t));
  pthread_mutex_init( lock_port_counter, NULL);
  
  return;
}







