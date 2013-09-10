#ifndef RESPARSE_H_INCLUDED
#define RESPARSE_H_INCLUDED

#include <setjmp.h>

enum token {
  /* Skip 10 to accommodate 'enum res_kind' values. */
  Notoken = 10,
  Left,
  Right,
  Number,
  List,
  Topology,
  Resources,
  Access,
  Local,
  Remote,
  Accept,
  Return,
  Systems,
  Hardware,
  Grant,
  Revoke,
  Quit,
  Help,
  State,
  Children,
  Logical,
  Physical,
  Numa,
  Hostname,
  String,
};

extern jmp_buf res_parse_exception_context;

#endif
