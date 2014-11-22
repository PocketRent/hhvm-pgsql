#ifndef _INCL_PGSQL_H
#define _INCL_PGSQL_H

#include "pq.h"

#include "hphp/runtime/base/base-includes.h"

#ifdef NEWOBJ
#define NEWRES(type) NEWOBJ(type)
#else
#define NEWRES(type) newres<type>
#endif

#endif
