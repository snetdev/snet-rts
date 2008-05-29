/*******************************************************************************
 *
 * $Id$
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
 * Implementation of the dispatcher used by SNet observer system.
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

#include "memfun.h"
#include "bool.h"

/* This struct stores information of sleeping observer. */
typedef struct sleeper{
  int id;               // ID of the sleeping observer .
  pthread_cond_t cond;  // Condition that the observer is waiting for.
  bool isSleeping;      // This value tells if the observer is actually sleeping.
  struct sleeper *next; // Next entry in sleeping queue.
}sleeper_t;

/* Size of buffer used to transfer unhanled data from received message to next message. */
#define MSG_BUF_SIZE 256

/* Size of notification buffer. */
#define NOTIFICATION_BUFFER_SIZE 4

/* Size of the buffer used to receive data. */
#define BUF_SIZE 256

/* This struct store information of a connection. */
typedef struct connection{
  int fdesc;                 // Socket file descriptor.
  struct hostent *host;      // Host where the socket is connected to.
  struct sockaddr_in addr;   // Socket address.
  sleeper_t *sleepers;       // Sleeping queue.
  struct connection *next;   // Next entry in connection list.
  char buffer[MSG_BUF_SIZE]; // This buffer is used to store possible extra characters incase
                             // if a part of the message received is missing.
}connection_t;

/* Thread for dispatcher. This is needed for pthread_join later. */
static pthread_t *dispatcher_thread;

/* Mutex to guard connections list */
static pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Connections list */
static connection_t *connections = NULL;

static int user_count = 0;

/* This value indicates if the dispatcher should terminate */
static volatile bool terminate = false;

/* This pipe is used to send messages to dispatcher thread. (Wake it up from select). */
static int notification_pipe[2];

/* Pattern of reply message for regex. */
static const char pattern[] = "^(<[\?]xml[ \n\t]+version=\"1[.]0\"[ \n\t]*[\?]>)?<data[ \n\t]+xmlns=\"snet-home[.]org\"[ \n\t]*>[ \n\t]*<observer[ \n\t]+oid=\"([0-9]+)\"[ \n\t]*((/>)|(>[ \n\t]*</observer[ \n\t]*>))[ \n\t]*</data[ \n\t]*>[ \n\t]*";

/* Compiled regex */
static regex_t *preg = NULL;


/* Open socket to address addr in port port. */
static connection_t *init_socket(const char *addr, int port)
{
  connection_t *temp = SNetMemAlloc(sizeof(connection_t));
  int i = 0;

  if(temp == NULL){
    return NULL;
  }

  temp->sleepers = NULL;
  temp->next = NULL;

  /* Open connection */
  if(!inet_aton(addr, &temp->addr.sin_addr)) {
    temp->host = gethostbyname(addr);

    if (temp->host) {
      temp->addr.sin_addr = *(struct in_addr*)temp->host->h_addr;
      temp->fdesc = socket(PF_INET, SOCK_STREAM, 0);

      if(temp->fdesc != -1){

	temp->addr.sin_port = htons(port);
	temp->addr.sin_family = AF_INET;

	if(connect(temp->fdesc, (struct sockaddr*)&temp->addr, sizeof(temp->addr)) != -1) {

	  /* Zero the message buffer. */
	  for(i = 0; i < MSG_BUF_SIZE; i++){
	    temp->buffer[i] = '\0';
	  }

	  /* Wake up the dispatcher thread in select. A new file has to be added! */
	  write(notification_pipe[1], "?", 1);
	  return temp;
	}
      }
    }
  }

  SNetMemFree(temp);

  return NULL;
}


/* Add sleeper entry to connection. */
static void add_sleeper(int self, connection_t *con)
{
  sleeper_t *temp = SNetMemAlloc( sizeof(sleeper_t));

  if(temp == NULL){
    return;
  }

  pthread_cond_init(&temp->cond, NULL);
  temp->id = self;
  temp->isSleeping = false;
  temp->next = NULL;
  
  temp->next = con->sleepers; 
  con->sleepers = temp;
}


/* Remove sleeper entry from connection. */
static void remove_sleeper(int self, connection_t *con)
{
  sleeper_t *temp = con->sleepers;
  sleeper_t *prev = NULL;

  while(temp != NULL){
    if(temp->id == self){
      if(prev != NULL){
	prev->next = temp->next;
      }
      else{
	con->sleepers = temp->next;
      }
      pthread_cond_destroy(&temp->cond);
      SNetMemFree(temp);
      return;
    }
    prev = temp;
    temp = temp->next;
  }
}

/* Parse reply message using regex. 
 *
 */

static int parse_reply_msg(char *buf, int *oid)
{
  int ret = 0;
  regmatch_t pmatch[3];
  char id[32];
  int i = 0;
  *oid = -1;

  if(preg == NULL){
    return 0;
  }

  if((ret =  regexec(preg, buf, (size_t)3, pmatch, 0)) != 0){
    return 0;
  }

  /* Take oid. */
  for(i = pmatch[2].rm_so; i <= pmatch[2].rm_eo; i++){
    id[i - pmatch[2].rm_so] = buf[i];
  }
  id[i] = '\0';

  *oid = atoi(id);

  return pmatch[0].rm_eo;
}

/* This function is used by the dispatcher thread.
 * 
 */

static void *dispatcher_dispatch(void *arg) 
{
  fd_set fdr_set;
  int ndfs = 0;
  int ret;
  connection_t *temp = NULL;
  int len = 0;
  int i = 0;

  pthread_mutex_lock(&connection_mutex);

  /* Loop until the observer system is terminated */
  while(terminate == false ||  user_count > 0){
    /* Form the read set. */
    ndfs = 0;
    temp = NULL;
    FD_ZERO(&fdr_set);
    
    /* Notification pipe is used to end select. */
    temp = connections;
    FD_SET(notification_pipe[0], &fdr_set);
    ndfs = notification_pipe[0] + 1;

    while(temp != NULL){
      FD_SET(temp->fdesc, &fdr_set);
      ndfs = (temp->fdesc + 1 > ndfs) ? temp->fdesc + 1 : ndfs;
      temp = temp->next;
    }

    pthread_mutex_unlock(&connection_mutex);

    if(ndfs > 0){

      if((ret = select(ndfs, &fdr_set,  NULL, NULL, NULL)) == -1){

	/* Error. */
	pthread_mutex_lock(&connection_mutex);
      }
      else if(FD_ISSET(notification_pipe[0], &fdr_set)){
	char buffer[NOTIFICATION_BUFFER_SIZE];

	/* Something changed. What is written to the ontification pipe is meaningles. */

	read(notification_pipe[0], buffer, NOTIFICATION_BUFFER_SIZE);
	pthread_mutex_lock(&connection_mutex);
      }
      else{
	/* Incoming message(s). Go through all the changed files and parse messages */

	pthread_mutex_lock(&connection_mutex);
	temp = connections;

	while(temp != NULL){
	  if(FD_ISSET(temp->fdesc, &fdr_set)){
	    char buf[BUF_SIZE];
	    int buflen;

	    for(i = 0; i < strlen(temp->buffer); i++){
	      buf[i] = temp->buffer[i];
	    }

	    buflen = strlen(buf);
	    len = recv(temp->fdesc, buf + buflen, BUF_SIZE - buflen, 0);

	    buf[buflen + len] = '\0';

	    if(len > 0){
	      int oid;
	      int offset = 0;
	      int ret = 0;

	      /* Parse messages. There might be multiple messages. */
	      do{
		ret = parse_reply_msg(buf + offset, &oid);
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
		sleeper_t *sleep = temp->sleepers;
		while(sleep != NULL){
		  if(sleep->id == oid && sleep->isSleeping == true){
		    pthread_cond_signal(&sleep->cond);
		    break;
		  }
		  sleep = sleep->next;
		}
	      }while(1);
	    }
	  }
	  temp = temp->next;
	}
      }
    }else{
      pthread_mutex_lock(&connection_mutex);
    }
  }
  pthread_mutex_unlock(&connection_mutex);

  return NULL;
}


/* Register observer to the dispatcher */
int SNetDispatcherAdd(int self, const char *addr, int port, bool interactive)
{
  connection_t *temp = connections;
  connection_t *new = NULL; 
  struct hostent *host;

  pthread_mutex_lock(&connection_mutex);

  if(terminate == true){
    pthread_mutex_unlock(&connection_mutex);
    return -1;
  }

  /* Test if the connection has already been opened */
  host = gethostbyname(addr);

  while(temp != NULL){
    if((*(struct in_addr*)host->h_addr).s_addr == temp->addr.sin_addr.s_addr
       && temp->addr.sin_port == htons(port)) {

      if(interactive == true){
	add_sleeper(self, temp);
      }

      user_count++;
      pthread_mutex_unlock(&connection_mutex);
      return temp->fdesc;
    }
    temp = temp->next;
  }

  /* New connection */

  if((new = init_socket(addr, port)) == NULL) {
    pthread_mutex_unlock(&connection_mutex);
    return -1;
  }

  temp = connections;
  new->next = connections;
  connections = new;

  if(interactive == true){
    add_sleeper(self, new);
  }
  
  user_count++;
  pthread_mutex_unlock(&connection_mutex);
  return new->fdesc;
}

/* Unregister observer from the dispatcher. */
void SNetDispatcherRemove(int self, int fid)
{
  connection_t *temp = connections;
  connection_t *prev = NULL;

  pthread_mutex_lock(&connection_mutex);
 
  while(temp != NULL){
    if(temp->fdesc == fid){
      remove_sleeper(self, temp);

      /* The socket cannot be closed as some observer
       * created in the future might use it and closing
       * could affect the listening application.
       */ 
      user_count--;

      break;
    }

    prev = temp;
    temp = temp->next;
  } 

  pthread_mutex_unlock(&connection_mutex);
}


/* This is used by the observers to send anything out. Blocks the observer if its an interactive one. */
int SNetDispatcherSend(int self, int fid, const char *buffer, int buflen, bool interactive)
{
  int ret;
  connection_t *temp = connections;

  pthread_mutex_lock(&connection_mutex);

  if(terminate == true){
    pthread_mutex_unlock(&connection_mutex);
    return -1;
  }

  /* Search the right socket */
  while(temp != NULL){
    if(temp->fdesc == fid){
      break;
    }

    temp = temp->next;
  }

  /* Error: connection not registerd! */
  if(temp == NULL){
    pthread_mutex_unlock(&connection_mutex);
    return -1;
  }

  /* Send the data */
  ret = send(temp->fdesc, buffer, buflen, 0);

  /* Interactive observers block */
  if(interactive == true && ret >= 0){

    /* Add to sleeper list*/
    sleeper_t *sleeper = temp->sleepers;

    while(sleeper != NULL){
      if(sleeper->id == self){
	sleeper->isSleeping = true;

	if(pthread_cond_wait(&sleeper->cond, &connection_mutex) != 0)
	  {
	    /* Error */
	  }
	sleeper->isSleeping = false;
	break;
      }
      sleeper = sleeper->next;
    }
  }

  pthread_mutex_unlock(&connection_mutex);
  return ret; 
}


/* Init dispatcher system. Starts the dispatcher thread. */
int SNetDispatcherInit()
{
  /* Start dispatcher */ 
  terminate = 0;

  preg = SNetMemAlloc(sizeof(regex_t));

  if(preg == NULL){
    return -1; 
  }

  /* Compile regex pattern for parsing reply messages. */
  if(regcomp(preg, pattern, REG_EXTENDED) != 0){
    return -1;
  }
  
  /* Open notification pipe. */
  if(pipe(notification_pipe) == -1){
    return -1;
  }

  if((dispatcher_thread = SNetMemAlloc(sizeof(pthread_t))) == NULL)
  {
    return -1;
  }

  pthread_create(dispatcher_thread, NULL, dispatcher_dispatch, NULL);

  return 0;
}


/* Destroy dispatcher system. Waits unti lthe dispatcher come out of select
 * and then frees all reserved memory. 
 */
void SNetDispatcherDestroy()
{
  connection_t *con; 

  /* Don't close connections before the dispatcher stops using them! */
  
  pthread_mutex_lock(&connection_mutex);
  terminate = true;
  pthread_mutex_unlock(&connection_mutex);

  write(notification_pipe[1], "?", 1);

  if(pthread_join(*dispatcher_thread, NULL) != 0)
  {
    /* Error */
  }

  SNetMemFree(dispatcher_thread);

  con = connections;

  /* Destroy all connections. */
  while(con != NULL){
    connections = con->next;
    
    close(con->fdesc);
    
    SNetMemFree(con);
    
    con = connections;
  }

  regfree(preg);
}

#undef MSG_BUF_SIZE 
#undef NOTIFICATION_BUFFER_SIZE
#undef BUF_SIZE 
