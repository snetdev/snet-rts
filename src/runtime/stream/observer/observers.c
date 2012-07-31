/*******************************************************************************
 *
 * $Id: observers.c 2891 2010-10-27 14:48:34Z dlp $
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   4.4.2008
 * -----
 *
 * Description:
 * ------------
 *
 * SNet observer implementation
 *
 *******************************************************************************/

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <regex.h>

#include "bool.h"
#include "constants.h"
#include "memfun.h"
#include "bool.h"
#include "interface_functions.h"
#include "observers.h"

#include "threading.h"

#include "debug.h"
#include "distribution.h"

/* Size of buffer used to transfer unhanled data from received message to next message. */
#define MSG_BUF_SIZE 256

/* Size of notification buffer. */
#define NOTIFICATION_BUFFER_SIZE 4

/* Size of the buffer used to receive data. */
#define BUF_SIZE 256

/* Observer type. */
typedef enum {OBSfile, OBSsocket} obstype_t;


/* A handle for observer data. */
typedef struct obs_handle {
  obstype_t obstype;    // type of the observer
  void *desc;           // Observer descriptor, depends on the type of the observer (obs_socket_t / obs_file_T)
  bool isInteractive;   // Is this observer interactive
  snet_stream_t *inbuf;      // SNetStream for incoming records
  snet_stream_t *outbuf;     // SNetStream for outgoing records
  int id;               // Instance specific ID of this observer (NOTICE: this will change!)
  const char *code;     // Code attribute
  const char *position; // Name of the net/box to which the observer is attached to
  char type;            // Type of the observer (before/after)
  char data_level;      // Data level used
}obs_handle_t;


/* Wait queue for observers waiting for reply messages */
typedef struct obs_wait_queue {
  bool hasBeenWoken;
  obs_handle_t *hnd;           // Observer
  pthread_cond_t cond;         // Wait condition
  struct obs_wait_queue *next;
} obs_wait_queue_t;


/* Data for socket */
typedef struct obs_socket{
  int fdesc;                    // Socket file descriptor
  FILE *file;                   // File pointer to the file descriptor
  int users;                    // Number of users currently registered for this socket.
  struct hostent *host;         // Socket data
  struct sockaddr_in addr;
  obs_wait_queue_t *wait_queue; // Queue of currently waiting observers.
  char buffer[MSG_BUF_SIZE];    // Buffer for buffering leftovers of last message
  struct obs_socket *next;
}obs_socket_t;


/* Data for file */
typedef struct obs_file {
  FILE *file;             // File
  int users;              // Number of users currently registered for this file.
  char *filename;         // Name of the file
  struct obs_file *next;
} obs_file_t;



/* Thread for dispatcher. This is needed for pthread_join later. */
static pthread_t dispatcher_thread;

/* Mutex to guard connections list */
static pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Registered sockets / files */
static obs_socket_t *open_sockets = NULL;
static obs_file_t *open_files = NULL;
static int user_count = 0;


/* This value indicates if the dispatcher should terminate */
static volatile bool mustTerminate = false;

/* This pipe is used to send messages to dispatcher thread. (Wake it up from select). */
static int notification_pipe[2];

/* Pattern of reply message for regex. */
static const char pattern[] = "^(<[\?]xml[ \n\t]+version=\"1[.]0\"[ \n\t]*[\?]>)?<observer[ \n\t]+(xmlns=\"snet-home[.]org\"[ \n\t]+)?oid=\"([0-9]+)\"([ \n\t]+xmlns=\"snet-home[.]org\"[ \n\t]+)?[ \n\t]*((/>)|(>[ \n\t]*</observer[ \n\t]*>))[ \n\t]*";

/* Compiled regex */
static regex_t *preg = NULL;

/* These are used to provide instance specific IDs for observers */
static int id_pool = 0;

static snetin_label_t *labels = NULL;
static snetin_interface_t *interfaces = NULL;



/** <!--********************************************************************-->
 *
 * @fn obs_socket_t *ObserverInitSocket(const char *addr, int port)
 *
 *   @brief  Open new socket for an observer.
 *
 *           The socket is automatically added to the list of sockets
 *           and the dispatcher is notified of new socket.
 *
 *   @param addr  The address of the listener
 *   @param port  The port used by the listener
 *   @return      The new socket, or NULL if the operation failed.
 *
 ******************************************************************************/

static obs_socket_t *ObserverInitSocket(const char *addr, int port)
{
  obs_socket_t *new = SNetMemAlloc(sizeof(obs_socket_t));
  unsigned int i = 0;
  int res;

  if(new == NULL){
    SNetUtilDebugNotice("Observer: socket initialization error");
    return NULL;
  }

  new->wait_queue = NULL;
  new->next = NULL;

  /* Open connection */
  if(inet_aton(addr, &new->addr.sin_addr)) {
    SNetMemFree(new);
    SNetUtilDebugNotice("Observer: socket address error");
    return NULL;
  }

  new->host = gethostbyname(addr);

  if (!new->host) {
    SNetMemFree(new);
    SNetUtilDebugNotice("Observer: socket hostname error");
    return NULL;
  }

  new->addr.sin_addr = *(struct in_addr*)new->host->h_addr;
  new->fdesc = socket(PF_INET, SOCK_STREAM, 0);

  if(new->fdesc == -1){
    SNetMemFree(new);
    SNetUtilDebugNotice("Observer: socket descriptor error");
    return NULL;
  }

  new->addr.sin_port = htons(port);
  new->addr.sin_family = AF_INET;

  if(connect(new->fdesc, (struct sockaddr*)&new->addr, sizeof(new->addr)) == -1) {
    SNetMemFree(new);
    // Could not connect to the socket -> Ignore this observer
    return NULL;
  }

  new->file = fdopen(new->fdesc, "w");

  if(new->file == NULL) {
    SNetMemFree(new);
    SNetUtilDebugNotice("Observer: socket descriptor error");
    return NULL;
  }

  /* Zero the message buffer. */
  for(i = 0; i < MSG_BUF_SIZE; i++){
    new->buffer[i] = '\0';
  }

  /* Wake up the dispatcher thread in select. A new file has to be added! */
  res = write(notification_pipe[1], "?", 1);
  if (res == -1) {
    SNetUtilDebugFatal("Observer: write to notification pipe failed (%s)", strerror(errno));
  }

  new->next = open_sockets;
  open_sockets = new;

  return new;
}

/** <!--********************************************************************-->
 *
 * @fn void ObserverWait(obs_handle_t *self)
 *
 *   @brief  Put observer to wait for a reply message.
 *
 *           Observer blocks until it is released by the dispatcher.
 *
 *   @param self  Handle of the observer.
 *
 ******************************************************************************/

static void ObserverWait(obs_handle_t *self)
{
  obs_socket_t *socket;
  obs_wait_queue_t *temp;

  if(self == NULL || self->obstype != OBSsocket || self->desc == NULL) {
    return;
  }

  /* Search for unused wait queue places. */

  socket = (obs_socket_t *)self->desc;

  temp = socket->wait_queue;

  while(temp != NULL) {
    if(temp->hnd == NULL) {
      temp->hnd = self;

      temp->hasBeenWoken = false;

      while(temp->hasBeenWoken == false) {
	pthread_cond_wait(&temp->cond, &connection_mutex);
      }

      temp->hnd = NULL;

      return;
    }

    temp = temp->next;
  }

  /* No free wait queue structs. New one is created. */

  temp = SNetMemAlloc( sizeof(obs_wait_queue_t));

  if(temp == NULL){
    return;
  }

  pthread_cond_init(&temp->cond, NULL);
  temp->hnd = self;
  temp->next = socket->wait_queue;
  socket->wait_queue = temp;


  temp->hasBeenWoken = false;

  while(temp->hasBeenWoken == false) {
    pthread_cond_wait(&temp->cond, &connection_mutex);
  }

  temp->hnd = NULL;

  return;
}

/** <!--********************************************************************-->
 *
 * @fn int ObserverParseReplyMessage(char *buf, int *oid)
 *
 *   @brief  This function parses a reply message and returns ID of the
 *           corresponding observer.
 *
 *   @param buf   Buffer containing the message.
 *   @param oid   Variable to where the ID is stored.
 *   @return      Number of parsed characters.
 *
 ******************************************************************************/

static int ObserverParseReplyMessage(char *buf, int *oid)
{
  int ret = 0;
  regmatch_t pmatch[4];
  char id[32];
  int i = 0;
  *oid = -1;

  if(preg == NULL){
    return 0;
  }

  if((ret =  regexec(preg, buf, (size_t)4, pmatch, 0)) != 0){
    return 0;
  }

  /* Take oid. */
  for(i = pmatch[3].rm_so; i < pmatch[3].rm_eo; i++){
    id[i - pmatch[3].rm_so] = buf[i];
  }
  id[i - pmatch[3].rm_so] = '\0';

  *oid = atoi(id);

  return pmatch[0].rm_eo;
}

/** <!--********************************************************************-->
 *
 * @fn void *ObserverDispatch(void *arg)
 *
 *   @brief  This is the main function for the dispatcher thread.
 *
 *           Dispatcher listens to all open sockets and parses possible reply
 *           messages. Blocked interactive observers are released when
 *           corresponding reply message is received.
 *
 *   @param arg   Possible data structures needed by the dispatcher
 *   @return      Always returns NULL
 *
 ******************************************************************************/

static void *ObserverDispatch(void *arg)
{
  fd_set fdr_set;
  int ndfs = 0;
  int ret;
  obs_socket_t *temp = NULL;
  int len = 0;
  unsigned int i = 0;
  (void) arg; /* NOT USED */
  pthread_mutex_lock(&connection_mutex);

  /* Loop until the observer system is terminated */
  while(mustTerminate == false ||  user_count > 0){
    /* Form the read set. */
    ndfs = 0;
    temp = NULL;
    FD_ZERO(&fdr_set);

    /* Notification pipe is used to end select. */
    temp = open_sockets;
    FD_SET(notification_pipe[0], &fdr_set);
    ndfs = notification_pipe[0] + 1;

    while(temp != NULL){
      FD_SET(temp->fdesc, &fdr_set);
      ndfs = (temp->fdesc + 1 > ndfs) ? temp->fdesc + 1 : ndfs;

      temp = temp->next;
    }

    if(ndfs > 0){

      pthread_mutex_unlock(&connection_mutex);

      ret = select(ndfs, &fdr_set,  NULL, NULL, NULL);

      pthread_mutex_lock(&connection_mutex);

      if(ret == -1){

	/* Error. */
      }
      else if(FD_ISSET(notification_pipe[0], &fdr_set)){
	char buffer[NOTIFICATION_BUFFER_SIZE];
        int res;

	/* Something changed. What is written to the ontification pipe is meaningles. */

	res = read(notification_pipe[0], buffer, NOTIFICATION_BUFFER_SIZE);
        if (res == -1) {
          SNetUtilDebugFatal("Observer: read from notification pipe failed (%s)", strerror(errno));
        }
      }
      else{
	/* Incoming message(s). Go through all the changed files and parse messages */

	temp = open_sockets;

	while(temp != NULL){
	  if(FD_ISSET(temp->fdesc, &fdr_set)){
	    char buf[BUF_SIZE];
	    int buflen;

	    for (i = 0; i < strlen(temp->buffer); i++){
	      buf[i] = temp->buffer[i];
	    }

	    buflen = strlen(temp->buffer);

	    len = recv(temp->fdesc, buf + buflen, BUF_SIZE - buflen, 0);

	    buf[buflen + len] = '\0';

	    if(len > 0){
	      int oid;
	      int offset = 0;
	      int ret = 0;

	      /* Parse messages. There might be multiple messages. */
	      do{
		ret = ObserverParseReplyMessage(buf + offset, &oid);
		offset += ret;

		/* Nothing was parsed. Buffer the message to be used
		 * when the next messge arrives. */
		if(ret == 0 || oid == -1){
		  for(i = 0; i < strlen(buf);i++){
		    temp->buffer[i] = buf[offset + i];
		  }

		  for(; i < 128; i++){
		    temp->buffer[i] = '\0';
		  }
		  break;
		}

		/* Wake up sleeping observer */
		obs_wait_queue_t *queue = temp->wait_queue;
		while(queue != NULL){
		  if(queue->hnd != NULL && queue->hnd->id == oid) {
		    queue->hasBeenWoken = true;
		    pthread_cond_signal(&queue->cond);
		    break;
		  }
		  queue = queue->next;
		}
	      }while(1);
	    }
	  }
	  temp = temp->next;
	}
      }
    }
  }

  pthread_mutex_unlock(&connection_mutex);

  return NULL;
}

/** <!--********************************************************************-->
 *
 * @fn int ObserverRegisterSocket(obs_handle_t *self,  const char *addr, int port)
 *
 *   @brief  Register an observer for socket output connection.
 *
 *   @param self  Observer handle of the observer to be registered.
 *   @param addr  The address of the listener
 *   @param port  The port used by the listener
 *   @return      0 if the operation succeed, -1 otherwise.
 *
 ******************************************************************************/

static int ObserverRegisterSocket(obs_handle_t *self,  const char *addr, int port)
{
  obs_socket_t *temp = NULL;
  struct hostent *host;

  pthread_mutex_lock(&connection_mutex);

  /*
  if(mustTerminate == true){
    pthread_mutex_unlock(&connection_mutex);
    SNetUtilDebugNotice("Observer fatal register error");
    return -1;
  }
  */

  temp = open_sockets;

  /* Test if the connection has already been opened */
  host = gethostbyname(addr);

  while(temp != NULL){
    if((*(struct in_addr*)host->h_addr).s_addr == temp->addr.sin_addr.s_addr
       && temp->addr.sin_port == htons(port)) {

      user_count++;
      temp->users++;

      self->obstype = OBSsocket;
      self->desc = (void *)temp;

      pthread_mutex_unlock(&connection_mutex);

      return 0;
    }
    temp = temp->next;
  }

  /* New connection */

  if((temp = ObserverInitSocket(addr, port)) == NULL) {
    pthread_mutex_unlock(&connection_mutex);

    return -1;
  }

  user_count++;
  temp->users = 1;

  self->obstype = OBSsocket;
  self->desc = (void *)temp;

  pthread_mutex_unlock(&connection_mutex);

  return 0;
}

/** <!--********************************************************************-->
 *
 * @fn int ObserverRegisterFile(obs_handle_t *self, const char *filename)
 *
 *   @brief  Register an observer for output file.
 *
 *   @param self      Observer handle of the observer to be registered.
 *   @param filename  The name of the file to be used.
 *   @return          0 if the operation succeed, -1 otherwise.
 *
 ******************************************************************************/

static int ObserverRegisterFile(obs_handle_t *self, const char *filename)
{
  obs_file_t *temp;
  FILE *file = NULL;

  pthread_mutex_lock(&connection_mutex);

  /*
  if(mustTerminate == true){
    pthread_mutex_unlock(&connection_mutex);
    SNetUtilDebugNotice("Observer fatal register error");
    return -1;
  }
  */
  temp = open_files;

  while(temp != NULL){
    if(strcmp(temp->filename, filename) == 0) {

      user_count++;
      temp->users++;

      self->obstype = OBSfile;
      self->desc = (void *)temp;

      pthread_mutex_unlock(&connection_mutex);
      return 0;
    }

    temp = temp->next;
  }

  /* The file is not open yet: */

  file = fopen(filename, "a");

  if(file == NULL) {
    pthread_mutex_unlock(&connection_mutex);
    SNetUtilDebugNotice("Observer: file descriptor error");
    return -1;
  }

  temp = SNetMemAlloc(sizeof(obs_file_t));

  if(temp == NULL) {
    fclose(file);
    pthread_mutex_unlock(&connection_mutex);
    SNetUtilDebugNotice("Observer: socket memory error");
    return -1;
  }

  temp->file = file;

  user_count++;
  temp->users = 1;


  temp->filename = SNetMemAlloc(sizeof(char) * (strlen(filename) + 1));

  if(temp->filename == NULL) {
    fclose(file);
    pthread_mutex_unlock(&connection_mutex);
    SNetUtilDebugNotice("Observer: socket memory error");
    return -1;
  }

  temp->filename = strcpy(temp->filename, filename);

  temp->next = open_files;
  open_files = temp;

  self->obstype = OBSfile;
  self->desc = (void *)temp;

  pthread_mutex_unlock(&connection_mutex);
  return 0;
}

/** <!--********************************************************************-->
 *
 * @fn void ObserverRemove(obs_handle_t *self)
 *
 *   @brief  Unregister observer from the observer system.
 *
 *   @param self      Observer handle of the observer to be unregistered.
 *
 ******************************************************************************/

static void ObserverRemove(obs_handle_t *self)
{
  obs_file_t *file;
  obs_socket_t *socket;

  if(self == NULL) {
    return;
  }

  pthread_mutex_lock(&connection_mutex);

  if(self->obstype == OBSfile) {
    if(self->desc != NULL) {
      file = (obs_file_t *)self->desc;

      file->users--;
      user_count--;

    }
  } else if(self->obstype == OBSsocket) {
    if(self->desc != NULL) {
      socket = (obs_socket_t *)self->desc;

      socket->users--;
      user_count--;

    }
  }

  pthread_mutex_unlock(&connection_mutex);

  return;
}

/** <!--********************************************************************-->
 *
 * @fn int ObserverPrintRecordToFile(FILE *file, obs_handle_t *hnd, snet_record_t *rec)
 *
 *   @brief  Write record into the given output stream.

 *
 *   @param file  File pointer to where the record is printed.
 *   @param hnd   Handle of the observer.
 *   @param rec   Record to be printed.
 *   @return      0 in case of success, -1 in case of failure.
 *
 ******************************************************************************/

static int ObserverPrintRecordToFile(FILE *file, obs_handle_t *hnd, snet_record_t *rec)
{
  int name, val;
  snet_ref_t *field;
  char *label = NULL;
  char *interface = NULL;
  snet_record_mode_t mode;

  fprintf(file,"<?xml version=\"1.0\"?>\n");
  fprintf(file,"<observer xmlns=\"snet-home.org\" oid=\"%d\" position=\"%s\" type=\"", hnd->id, hnd->position);

  if(hnd->type == SNET_OBSERVERS_TYPE_BEFORE) {
    fprintf(file,"before\" ");
  } else if(hnd->type == SNET_OBSERVERS_TYPE_AFTER){
    fprintf(file,"after\" ");
  }

  if(hnd->code != NULL){
    fprintf(file,"code=\"%s\" ", hnd->code);
  }

  fprintf(file,">\n");

  switch(SNetRecGetDescriptor( rec)) {

  case REC_data:
    mode = SNetRecGetDataMode(rec);

    if(mode == MODE_textual) {
      fprintf(file, "<record type=\"data\" mode=\"textual\" >\n");
    }else {
      fprintf(file, "<record type=\"data\" mode=\"binary\" >\n");
    }

    int id = SNetRecGetInterfaceId(rec);
    /* fields */
    RECORD_FOR_EACH_FIELD(rec, name, field) {
      if((label = SNetInIdToLabel(labels, name)) != NULL){
        if(hnd->data_level == SNET_OBSERVERS_DATA_LEVEL_ALLVALUES
           && (interface = SNetInIdToInterface(interfaces, id)) != NULL){
          fprintf(file,"<field label=\"%s\" interface=\"%s\" >", label, interface);

          if(mode == MODE_textual) {
            SNetInterfaceGet(id)->serialisefun(file, SNetRefGetData(field));
          } else {
            SNetInterfaceGet(id)->encodefun(file, SNetRefGetData(field));
          }

          fprintf(file,"</field>\n");
        } else {
          fprintf(file,"<field label=\"%s\" />\n", label);
        }
      }
    }

    /* tags */
    RECORD_FOR_EACH_TAG(rec, name, val) {
      if((label = SNetInIdToLabel(labels, val)) != NULL){
        if(hnd->data_level == SNET_OBSERVERS_DATA_LEVEL_LABELS) {
          fprintf(file,"<tag label=\"%s\" />\n", label);
        }else {
          fprintf(file,"<tag label=\"%s\" >%d</tag>\n", label, val);
        }
      }
      SNetMemFree(label);
    }

    /* btags */
    RECORD_FOR_EACH_BTAG(rec, name, val) {
      if((label = SNetInIdToLabel(labels, name)) != NULL){
        if(hnd->data_level == SNET_OBSERVERS_DATA_LEVEL_LABELS) {
          fprintf(file,"<btag label=\"%s\" />\n", label);
        }else {
          fprintf(file,"<btag label=\"%s\" >%d</btag>\n", label, val);
        }
      }
      SNetMemFree(label);
    }

    fprintf(file,"</record>\n");
    break;
  case REC_sync:
    fprintf(file,"<record type=\"sync\" />\n");
  case REC_collect:
    fprintf(file,"<record type=\"collect\" />\n");
  case REC_sort_end:
    fprintf(file,"<record type=\"sort_end\" />\n");
  case REC_terminate:
    fprintf(file,"<record type=\"terminate\" />\n");
    break;
  default:
    break;
  }
  fprintf(file,"</observer>\n");
  fflush(file);

  return 0;
}

/** <!--********************************************************************-->
 *
 * @fn int ObserverSend(obs_handle_t *hnd, snet_record_t *rec)
 *
 *   @brief  Send record to the listener.

 *
 *   @param hnd   Handle of the observer.
 *   @param rec   Record to be sent.
 *   @return      0 in case of success, -1 in case of failure.
 *
 ******************************************************************************/

static int ObserverSend(obs_handle_t *hnd, snet_record_t *rec)
{
  int ret;
  int err;
  obs_socket_t *socket;
  obs_file_t *file;

  pthread_mutex_lock(&connection_mutex);

  if(/*mustTerminate == true ||*/ hnd == NULL || hnd->desc == NULL){
    pthread_mutex_unlock(&connection_mutex);
    SNetUtilDebugNotice("Observer fatal error");
    return -1;
  }

  if(hnd->obstype == OBSfile) {
    file = (obs_file_t *)hnd->desc;

    ret = ObserverPrintRecordToFile(file->file, hnd, rec);


    err = ferror(file->file);

    pthread_mutex_unlock(&connection_mutex);

    if(err) {
      SNetUtilDebugNotice("Observer %d: file write error (%d)", hnd->id, err);
    }

    return ret;

  } else if(hnd->obstype == OBSsocket) {
    socket = (obs_socket_t *)hnd->desc;

    ret = ObserverPrintRecordToFile(socket->file, hnd, rec);
    fflush(socket->file);


    if(hnd->isInteractive == true){
      ObserverWait(hnd);
    }

    err = ferror(socket->file);

    pthread_mutex_unlock(&connection_mutex);

    if(err) {
      SNetUtilDebugNotice("Observer %d: socket send error (%d)", hnd->id, err);
    }

    return ret;
  }

  pthread_mutex_unlock(&connection_mutex);
  return -1;
}

/** <!--********************************************************************-->
 *
 * @fn void ObserverBoxTask( void *arg) {
 *
 *   @brief  The main function for observer task.
 *
 *           Observer takes incoming records, sends them to the listener,
 *           and after that passes the record to the next entity.
 *
 *   @param hndl  Handle of the observer.
 *
 ******************************************************************************/

static void ObserverBoxTask(void *arg)
{
  obs_handle_t *hnd = (obs_handle_t*)arg;
  snet_record_t *rec = NULL;
  bool terminate = false;
  snet_stream_desc_t *instream, *outstream;

  instream  = SNetStreamOpen(hnd->inbuf,  'r');
  outstream = SNetStreamOpen(hnd->outbuf, 'w');

  /* Do until terminate record is processed. */
  while(!terminate) {
    rec = SNetStreamRead( instream);
    if(rec != NULL) {

      /* Send message. */
      if(ObserverSend(hnd, rec)) {
	SNetUtilDebugNotice("Observer send error");
      }

      if(SNetRecGetDescriptor(rec) == REC_terminate){
	terminate = true;
      }

      /* Pass the record to next component. */
      SNetStreamWrite( outstream, rec);
    }
  }

  /* Unregister observer */
  ObserverRemove(hnd);

  SNetStreamClose( instream, true);
  SNetStreamClose( outstream, false);

  SNetMemFree(hnd);
}



/**
 * Create an observer task,
 * on a wrapper thread
 */
static void CreateObserverTask( obs_handle_t *hnd)
{
  char name[16];
  (void) snprintf(name, 16, "observer%02d", hnd->id);
  /* create a detached wrapper thread */
  SNetThreadingSpawn( ENTITY_other, -1, NULL,
        name, ObserverBoxTask, hnd);
}





/** <!--********************************************************************-->
 *
 * @fn snet_stream_t *SNetObserverSocketBox(snet_stream_t *input,
 *				     snet_info_t *info,
 *				     int location,
 *				     const char *addr,
 *				     int port,
 *				     bool isInteractive,
 *				     const char *position,
 *				     char type,
 *                                   char data_level,
 *                                   const char *code)
 *
 *   @brief  Initialize an observer that uses socket communication.
 *
 *           Observer handle is created and the observer is started.
 *           Incase the connection cannot be opened, this observer is ignored.
 *
 *   @param input         SNetStream for incoming records.
 *   @param info          Info struct.
 *   @param location      Location to where this component is built.
 *   @param addr          Address of the listener.
 *   @param port          Port number that the listener uses.
 *   @param isInteractive True if this observer is interactive, false otherwise.
 *   @param position      String describing the position of this observer (Net/box name).
 *   @param type          Type of the observer SNET_OBSERVERS_TYPE_BEFORE or SNET_OBSERVERS_TYPE_AFTER
 *   @param data_level    Level of data sent by the observer: SNET_OBSERVERS_DATA_LEVEL_LABELS or SNET_OBSERVERS_DATA_LEVEL_TAGVALUES or SNET_OBSERVERS_DATA_LEVEL_ALLVALUES
 *   @param code          User specified code sent by the observer.
 *
 *   @return              SNetStream for outcoming records.
 *
 ******************************************************************************/
snet_stream_t *SNetObserverSocketBox( snet_stream_t *input,
                                        snet_info_t *info,
                                        int location,
                                        const char *addr,
                                        int port,
                                        bool isInteractive,
                                        const char *position,
                                        char type,
                                        char data_level,
                                        const char *code)
{
  obs_handle_t *hnd;
  snet_stream_t *output = NULL;

  input = SNetRouteUpdate(info, input, location);

  if(SNetDistribIsNodeLocation(location)) {
    hnd = SNetMemAlloc(sizeof(obs_handle_t));

    if(hnd == NULL){
      SNetMemFree(hnd);
      return input;
    }

    hnd->inbuf = input;

    pthread_mutex_lock(&connection_mutex);
    hnd->id = id_pool++;
    pthread_mutex_unlock(&connection_mutex);

    hnd->obstype = OBSsocket;

    /* Register observer */
    if(ObserverRegisterSocket(hnd, addr, port) != 0) {
      SNetMemFree(hnd);
      return input;
    }

    hnd->outbuf  = SNetStreamCreate(0);

    hnd->type = type;
    hnd->isInteractive = isInteractive;
    hnd->position = position;
    hnd->data_level = data_level;
    hnd->code = code;

    CreateObserverTask( hnd);

    output = (snet_stream_t*) hnd->outbuf;

  } else {
    output = input;
  }

  return output;
}

/** <!--********************************************************************-->
 *
 * @fn snet_stream_t *SNetObserverFileBox(snet_stream_t *input,
 *				   snet_info_t *info,
 *				   int location,
 *				   const char *filename,
 *				   const char *position,
 *				   char type,
 *				   char data_level,
 *				   const char *code)
 *
 *   @brief  Initialize an observer that uses file for output.
 *
 *           Observer handle is created and the observer is started.
 *           Incase the file cannot be opened, this observer is ignored.
 *
 *   @param input         SNetStream for incoming records.
 *   @param info          Info struct.
 *   @param location      Location to where this component is built.
 *   @param filename      Name of the file to be used.
 *   @param position      String describing the position of this observer (Net/box name).
 *   @param type          Type of the observer SNET_OBSERVERS_TYPE_BEFORE or SNET_OBSERVERS_TYPE_AFTER
 *   @param data_level    Level of data sent by the observer: SNET_OBSERVERS_DATA_LEVEL_LABELS or SNET_OBSERVERS_DATA_LEVEL_TAGVALUES or SNET_OBSERVERS_DATA_LEVEL_ALLVALUES
 *   @param code          User specified code sent by the observer.
 *
 *   @return              SNetStream for outcoming records.
 *
 ******************************************************************************/
snet_stream_t *SNetObserverFileBox(snet_stream_t *input,
                                      snet_info_t *info,
                                      int location,
                                      const char *filename,
                                      const char *position,
                                      char type,
                                      char data_level,
                                      const char *code)
{
  obs_handle_t *hnd;
  snet_stream_t *output = NULL;

  input = SNetRouteUpdate(info, input, location);

  if(SNetDistribIsNodeLocation(location)) {
    hnd = SNetMemAlloc(sizeof(obs_handle_t));
    if(hnd == NULL){
      SNetMemFree(hnd);
      return input;
    }

    hnd->inbuf  = input;
    hnd->outbuf = SNetStreamCreate(0);

    pthread_mutex_lock(&connection_mutex);
    hnd->id = id_pool++;
    pthread_mutex_unlock(&connection_mutex);

    hnd->obstype = OBSfile;

    if(ObserverRegisterFile(hnd, filename) != 0) {
      SNetMemFree(hnd);
      return input;
    }

    hnd->type = type;
    hnd->isInteractive = false;
    hnd->position = position;
    hnd->data_level = data_level;
    hnd->code = code;

    CreateObserverTask( hnd);

    output = (snet_stream_t*) hnd->outbuf;

  } else {
    output = input;
  }

  return output;
}

/** <!--********************************************************************-->
 *
 * @fn int SNetObserverInit(snetin_label_t *labs, snetin_interface_t *interfs)
 *
 *   @brief  Call to this function initializes the observer system.
 *
 *           Notice: This function must be called before any other observer
 *           function!
 *
 *   @param labs     Labels used in the S-Net
 *   @param interfs  Interfaces used in the S-Net
 *   @return         0 if success, -1 otherwise.
 *
 ******************************************************************************/
int SNetObserverInit(snetin_label_t *labs, snetin_interface_t *interfs)
{
  /* Start dispatcher */
  mustTerminate = 0;
  labels = labs;
  interfaces = interfs;

  preg = SNetMemAlloc(sizeof(regex_t));

  if(preg == NULL){
    SNetUtilDebugNotice("Observer init error");
  }

  /* Compile regex pattern for parsing reply messages. */
  if(regcomp(preg, pattern, REG_EXTENDED) != 0){
    SNetUtilDebugNotice("Observer regex error");
    return -1;
  }

  /* Open notification pipe. */
  if(pipe(notification_pipe) == -1){
    SNetUtilDebugNotice("Observer pipe error");
  }

  pthread_create(&dispatcher_thread, NULL, ObserverDispatch, NULL);

  return 0;
}


/** <!--********************************************************************-->
 *
 * @fn void SNetObserverDestroy()
 *
 *   @brief  Call to this function destroys the observer system.
 *
 *           Notice: No observer functions must be called after this call!
 *
 *
 ******************************************************************************/
void SNetObserverDestroy()
{
  obs_socket_t *socket;
  obs_file_t *file;
  obs_wait_queue_t *queue;
  int res;

  /* Don't close connections before the dispatcher stops using them! */

  pthread_mutex_lock(&connection_mutex);
  mustTerminate = true;
  pthread_mutex_unlock(&connection_mutex);

  res = write(notification_pipe[1], "?", 1);
  if (res == -1) {
    SNetUtilDebugFatal("Observer: write to notification pipe failed (%s)", strerror(errno));
  }

  if(pthread_join(dispatcher_thread, NULL) != 0)
  {
    /* Error */
    SNetUtilDebugNotice("Observer destroy error");
  }

  while(open_sockets != NULL){
    socket = open_sockets->next;

    fclose(open_sockets->file);

    while(open_sockets->wait_queue != NULL) {
      queue = open_sockets->wait_queue->next;

      pthread_cond_destroy(&open_sockets->wait_queue->cond);

      SNetMemFree(open_sockets->wait_queue);

      open_sockets->wait_queue = queue;
    }

    SNetMemFree(open_sockets);

    open_sockets = socket;
  }

  while(open_files != NULL){
    file = open_files->next;

    fclose(open_files->file);

    SNetMemFree(open_files->filename);
    SNetMemFree(open_files);

    open_files = file;
  }

  regfree(preg);

  SNetMemFree(preg);

  return;
}





