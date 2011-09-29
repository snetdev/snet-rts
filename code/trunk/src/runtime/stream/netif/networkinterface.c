/*******************************************************************************
 *
 * $Id:$
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "output.h"
#include "input.h"
#include "observers.h"
#include "networkinterface.h"
#include "debug.h"
#include "threading.h"
#include "distribution.h"
#include "locvec.h"

static FILE *SNetInOpenFile(const char *file, const char *args)
{
  FILE *fileptr = fopen(file, args);

  if(fileptr == NULL) {
    SNetUtilDebugFatal("Could not open file \"%s\" with mode \"%s\"!\n", file, args);
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
    SNetUtilDebugFatal("Could not create socket for input!\n");
  }

  if(setsockopt(fdesc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    SNetUtilDebugFatal("Could not obtain socket for input!\n");
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  memset(addr.sin_zero, '\0', sizeof addr.sin_zero);

  if(bind(fdesc, (struct sockaddr *)&addr, sizeof addr) == -1) {
    SNetUtilDebugFatal("Could not bind to input port (%d)!\n", addr.sin_port);
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
    SNetUtilDebugFatal("Socket error!\n");
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
    SNetUtilDebugFatal("Could not parse output address (%s)!\n", address);
  }

  host = gethostbyname(address);

  if(!host) {
    SNetUtilDebugFatal("Unknown host for output (%s)!\n", address);
  }

  addr.sin_addr = *(struct in_addr*)host->h_addr;
  fdesc = socket(PF_INET, SOCK_STREAM, 0);

  if(fdesc == -1){
    SNetUtilDebugFatal("Could not create socket for output!\n");
  }
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;

  if(connect(fdesc, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    close(fdesc);
    SNetUtilDebugFatal("Could not connect to host (%s:%d)!\n", address, port);
  }

  file = fdopen(fdesc, "w");
  if(file == NULL) {
    close(fdesc);
    SNetUtilDebugFatal("Socket error!\n");
  }

  return file;
}

static void SNetInClose(FILE *file)
{
  fclose(file);
}


/**
 * Main starting entry point of the SNet program
 */
int SNetInRun(int argc, char **argv,
              char *static_labels[], int number_of_labels,
              char *static_interfaces[], int number_of_interfaces,
              snet_startup_fun_t fun)
{
  FILE *input = stdin;
  FILE *output = stdout;
  snet_stream_t *input_stream = NULL;
  snet_stream_t *output_stream = NULL;
  int i = 0;
  snet_info_t *info;
  snet_locvec_t *locvec;
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
      printf("\t-i <filename>\t\tInput from file.\n");
      printf("\t-I <port>\t\tInput from socket.\n");
      printf("\t-h \t\t\tDisplay this help text.\n");
      printf("\t-o <filename>\t\tOutput to file.\n");
      printf("\t-O <address:port>\tOutput to socket.\n");
      printf("\t-m <mon_level>\t\tSet monitoring level (LPEL only).\n");
      printf("\t-w <workers>\t\tSet number of workers (LPEL) / visible cores (PThread).\n");
      printf("\n");

      if(input != stdin && input != NULL) {
	SNetInClose(input);
      }

      if(output != stdout && output != NULL) {
	SNetInClose(output);
      }

      return 0;

    } else if(strcmp(argv[i], "-i") == 0 && input == stdin && i + 1 <= argc) {
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

	SNetUtilDebugFatal("Could not parse URL!\n");
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

    SNetUtilDebugFatal("");
  }

  if(output == NULL) {

    if(input != stdin && input != NULL) {
      SNetInClose(input);
    }

    SNetUtilDebugFatal("");
  }


  /* Actual SNet network interface main: */

  /* check for number of interfaces */
  if (0 == number_of_interfaces) {
    SNetUtilDebugFatal("No language interfaces were specified by the source program!");
  }

  labels     = SNetInLabelInit(static_labels, number_of_labels);
  interfaces = SNetInInterfaceInit(static_interfaces, number_of_interfaces);

  info = SNetInfoInit();

  SNetDistribInit(argc, argv, info);

  (void) SNetThreadingInit(argc, argv);

  SNetObserverInit(labels, interfaces);

  locvec = SNetLocvecCreate();
  SNetLocvecSet(info, locvec);

  input_stream = SNetStreamCreate(0);
  output_stream = fun(input_stream, info, 0);
  output_stream = SNetRouteUpdate(info, output_stream, 0);

  SNetDistribStart();

  if (SNetDistribIsRootNode()) {
    /* create output thread */
    SNetInOutputInit(output, labels, interfaces, output_stream);

    /* create input thread */
    SNetInInputInit(input, labels, interfaces, input_stream);
  }

  SNetDistribWaitExit(info);

  /* tell the threading layer that it is ok to shutdown,
     and wait until it has stopped such that it can be cleaned up */
  (void) SNetThreadingStop();

  (void) SNetThreadingCleanup();

  SNetInfoDestroy(info);

  SNetLocvecDestroy(locvec);

  /* destroy observers */
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
