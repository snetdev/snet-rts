#ifndef _SNET_SNETTYPES_H_
#define _SNET_SNETTYPES_H_

#include "handle.h"
#include "expression.h"
#include "filter.h"

/* Type for box function */
typedef snet_handle_t* (*snet_box_fun_t)(snet_handle_t *);

/* Maintaining execution realms */
typedef snet_handle_t* (*snet_exerealm_create_fun_t)(snet_handle_t *);
typedef snet_handle_t* (*snet_exerealm_update_fun_t)(snet_handle_t *);
typedef snet_handle_t* (*snet_exerealm_destroy_fun_t)(snet_handle_t *);

#endif /* _SNET_SNETTYPES_H_ */
