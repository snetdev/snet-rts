#ifndef RESSTREAM_H_INCLUDED
#define RESSTREAM_H_INCLUDED

struct buffer {
  /* Pointer to allocated memory. */
  char*         data;

  /* Size of buffer. */
  int           size;

  /* Where to read. */
  int           start;

  /* End of valid data. */
  int           end;
};

struct stream {
  int           fd;
  buffer_t      read;
  buffer_t      write;
};

#endif
