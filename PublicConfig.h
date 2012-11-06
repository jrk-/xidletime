#ifndef __PUBLICCONFIG_H
#define __PUBLICCONFIG_H

#include <pthread.h>
#include <X11/Xlib.h>
#include <dbus/dbus.h>

#include "GetOptions.h"

typedef struct PublicConfigT
    { pthread_spinlock_t   publiclock // don't touch this; use withPublicConfig;
    ; unsigned int         dynamic    // makePublicConfig & destroyPublicConfig
    ; Options            * options
    ; Display            * dpy
    ; DBusConnection     * dbusconn
    ;
    } PublicConfigT;

PublicConfigT * makePublicConfig ( PublicConfigT * );

void destroyPublicConfig ( PublicConfigT * );

void withPublicConfig ( PublicConfigT *, void ( * ) ( PublicConfigT * ) );

#endif
