/*******************************************************************************
 *
 * $Id: 
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   10.1.2008
 * -----
 *
 * Description:
 * ------------
 *
 * Main function for S-NET network interface.
 *
 *******************************************************************************/

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "record.h"
#include "parser.h"
#include "output.h"
#include "observers.h"
#include "networkinterface.h"

#define SNET_DEFAULT_BUFSIZE 10

static FILE *SNetInOpenFile(const char *file, const char *args)
{
  FILE *fileptr = fopen(file, args);

  if(fileptr == NULL) {
    printf("Abort: Could not open file \"%s\" with mode \"%s\"!\n", file, args);
  }

  return fileptr;
}

static FILE *SNetInOpenInputSocket(int port)
{
  struct sockaddr_in addr;  
  struct sockaddr_in in_addr; 
  int fdesc;
  FILE *file;
  int yes = 1;
  unsigned int sin_size;
  int in_fdesc;

  if ((fdesc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Abort: Could not create socket for input!\n");
    return NULL;
  }
    
  if(setsockopt(fdesc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    printf("Abort: Could not obtain socket for input!\n");
    return NULL;
  } 

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  memset(addr.sin_zero, '\0', sizeof addr.sin_zero);

  if(bind(fdesc, (struct sockaddr *)&addr, sizeof addr) == -1) {
    printf("Abort: Could not bind to input port (%d)!\n", addr.sin_port);
    return NULL;
  }

  
  /* Notice: This blocks the initialization of the SNet until the 
   * first connection! (Listen in different thread?) 
   */
  listen(fdesc, 1); 
  
  sin_size = sizeof(in_addr);
  
  in_fdesc = accept(fdesc, (struct sockaddr *)&in_addr, &sin_size);
  
  /* Currently only one connection accepted! 
   * Note: it could be a nice feature to accept more?  
   */
  
  close(fdesc);

  file = fdopen(in_fdesc, "r");
  
  if(file == NULL) {
    printf("Abort: Socket error!\n");
    return NULL;
  }
  
  return file;
}

static FILE *SNetInOpenOutputSocket(const char *address, int port)
{
  struct hostent *host;
  struct sockaddr_in addr; 
  int fdesc;
  FILE *file;

  if(inet_aton(address, &addr.sin_addr)) {
    printf("Abort: Could not parse output address (%s)!\n", address);
    return NULL;
  }

  host = gethostbyname(address);
    
  if(!host) {
    printf("Abort: Unknown host for output (%s)!\n", address);
    return NULL;
  }

  addr.sin_addr = *(struct in_addr*)host->h_addr;
  fdesc = socket(PF_INET, SOCK_STREAM, 0);
  
  if(fdesc == -1){
    printf("Abort: Could not create socket for output!\n");
    return NULL;
  }
	
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;
  
  if(connect(fdesc, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
	
    close(fdesc);
    printf("Abort: Could not connect to host (%s:%d)!\n", address, port);

    return NULL;
  }
  
  file = fdopen(fdesc, "w");
	  
  if(file == NULL) {
	
    close(fdesc);
    printf("Abort: Socket error!\n");

    return NULL;
  }

  return file;
}

static void SNetInClose(FILE *file)
{
  fclose(file);
}



int SNetInRun(int argc, char *argv[],
	      char *static_labels[], int number_of_labels, 
	      char *static_interfaces[], int number_of_interfaces, 
	      snet_tl_stream_t *(snetfun)(snet_tl_stream_t *))
{
  FILE *input = stdin;
  FILE *output = stdout;
  int bufsize = SNET_DEFAULT_BUFSIZE;
  int i = 0;
  snet_tl_stream_t *in_buf = SNetTlCreateStream(bufsize);
  snet_tl_stream_t *out_buf = NULL;
  snetin_label_t *labels = NULL;
  snetin_interface_t *interfaces = NULL;
  char *brk;
  char addr[256];
  int len;
  int port;

  /* Parse argv: */
  for(i = 1; i < argc; i++) {
    if(strcmp(argv[i], "-h") == 0) {
      /* Help */
      printf("usage: <executable name> [options]\n");
      printf("\nOptions:\n");
      printf("\t-b <buffer size>\tSet buffer size.\n");
      printf("\t-i <filename>\t\tInput from file.\n");
      printf("\t-I <port>\t\tInput from socket.\n");
      printf("\t-h \t\t\tDisplay this help text.\n");
      printf("\t-o <filename>\t\tOutput to file.\n");
      printf("\t-O <address:port>\tOutput to socket.\n");
      printf("\n");

      if(input != stdin && input != NULL) {
	SNetInClose(input);
      }
      
      if(output != stdout && output != NULL) {
	SNetInClose(output);
      }

      return 0;

    } else if(strcmp(argv[i], "-b") == 0 && i + 1 <= argc) {
      /* Buffer size */
      i = i + 1;
      bufsize = atoi(argv[i]);
    }else if(strcmp(argv[i], "-i") == 0 && input == stdin && i + 1 <= argc) {
      /* Input from file */
      i = i + 1;
      input =  SNetInOpenFile(argv[i], "r");
    } else if(strcmp(argv[i], "-I") == 0 && input == stdin && i + 1 <= argc) {
      /* Input from socket */
      i = i + 1;
      input = SNetInOpenInputSocket(atoi(argv[i]));
    } else if(strcmp(argv[i], "-o") == 0 && output == stdout && i + 1 <= argc) {
      /* Output to file */
      i = i + 1;
      output = SNetInOpenFile(argv[i], "w");
    } else if(strcmp(argv[i], "-O") == 0 && output == stdout && i + 1 <= argc) {
      /* Output to socket */
      i = i + 1;
      brk = strchr(argv[i], ':');

      if(brk == NULL) {
	output = NULL;

	printf("Abort: Could not parse URL!\n");
	break;
      }

      len = brk - argv[i];
      strncpy((char *)addr, (const char *)argv[i], len);
      addr[len] = '\0';
      port = atoi(brk + 1);

      output = SNetInOpenOutputSocket(addr, port);
    }
  }
 
  if(input == NULL) {

    if(output != stdout && output != NULL) {
      SNetInClose(output);
    }  
    return 1;
  }

  if(output == NULL) {

    if(input != stdin && input != NULL) {
      SNetInClose(input);
    }
    return 2;
  }

  if(bufsize <= 0) {

    if(input != stdin) {
      SNetInClose(input);
    }  
    if(output != stdout) {
      SNetInClose(output);
    }
    printf("Abort: negative or zero buffer size!\n");
    return 3;
  }
  

  /* Actual SNet network interface main: */

  labels     = SNetInLabelInit(static_labels, number_of_labels);
  interfaces = SNetInInterfaceInit(static_interfaces, number_of_interfaces);
  
  SNetObserverInit(labels, interfaces);
  
  out_buf = snetfun(in_buf);
  
  if(SNetInOutputInit(output, labels, interfaces, out_buf) != 0){
    /* TODO: free resources! */
    printf("Abort: Could not initialize output component!\n");
    return 4;
  }
  
  SNetInParserInit(input, labels, interfaces, in_buf);
 
  i = SNET_PARSE_CONTINUE;
  while(i != SNET_PARSE_TERMINATE){
    i = SNetInParserParse();
  }

  if(SNetInOutputDestroy() != 0){
    //return 1;
  }
  
  if(in_buf != NULL){
    SNetTlMarkObsolete(in_buf);
  }
  
  SNetInParserDestroy();
  
  SNetObserverDestroy();
  
  SNetInLabelDestroy(labels);
  SNetInInterfaceDestroy(interfaces);

  if(input != stdin) {
    SNetInClose(input);
  }

  if(output != stdout) {
    SNetInClose(output);
  }

  return 0;
}

#undef SNET_DEFAULT_BUFSIZE
