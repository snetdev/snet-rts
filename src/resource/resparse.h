#ifndef RESPARSE_H_INCLUDED
#define RESPARSE_H_INCLUDED

#include <setjmp.h>

enum token {
  Notoken,
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
};

extern jmp_buf res_parse_exception_context;

#endif
