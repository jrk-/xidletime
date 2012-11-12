#include "XIdleTimer.h"

#include <stdio.h>
#include <string.h>

#include <xcb/screensaver.h>

static XTimerT xtimer;

uint getXIdleTime ( void ) {
    return xtimer.idletime;
}

void setXIdleTime ( uint idletime ) {
    xtimer.idletime = idletime;
    xcb_set_screen_saver ( xtimer.c, idletime / 1000, 0, 0, 0 );
}

void suspendIdleTimer ( int suspend ) {
    xtimer.suspend = suspend;
}

static xcb_screen_t * screen_of_display ( xcb_connection_t * c, int screen ) {
    xcb_screen_iterator_t iter;

    iter = xcb_setup_roots_iterator ( xcb_get_setup ( c ) );
    for (; iter.rem; --screen, xcb_screen_next ( &iter ) )
        if ( screen == 0 )
            return iter.data;

    return NULL;
}

void * xIdleTimerSource ( EventSourceT * src ) {
    xcb_generic_event_t * event = NULL;
    xcb_screen_t        * screen = NULL;
    xcb_window_t          root = 0;
    int                   screen_default_nbr = 0;

    if ( src->public->c == NULL ) {
        src->public->c = xcb_connect ( NULL, &screen_default_nbr );
    }

    if ( src->private == NULL ) {
        memset ( &xtimer, 0, sizeof ( XTimerT ) );
        xtimer.c = src->public->c;
        setXIdleTime ( src->public->options->idletime );
        src->private = (void *)&xtimer;
    }

    screen = screen_of_display ( src->public->c, screen_default_nbr );
    if ( screen ) root = screen->root; else return NULL;

    const xcb_query_extension_reply_t * extreply =
        xcb_get_extension_data ( src->public->c, &xcb_screensaver_id );

    setXIdleTime ( src->public->options->idletime );
    xcb_screensaver_select_input ( src->public->c
                                 , root
                                 , XCB_SCREENSAVER_EVENT_NOTIFY_MASK
                                 );

    xcb_flush ( src->public->c );

    while ( ( event = xcb_wait_for_event ( src->public->c ) ) ) {
        if ( xtimer.suspend
          || event->response_type
                != extreply->first_event + XCB_SCREENSAVER_NOTIFY
           ) continue;

        if ( ((xcb_screensaver_notify_event_t *)event)->code
                == XCB_SCREENSAVER_STATE_ON
           ) {
            xtimer.status = Idle;
        } else {
            xtimer.status = Reset;
        }

        src->eq->queueEvent ( src->eq, src );
    }

    return NULL;
}

void xIdleTimerSink ( EventSinkT * snk, EventSourceT * src ) {
    if ( xtimer.status == Idle ) {
        fprintf ( stdout, "Idle\n" );
    } else {
        fprintf ( stdout, "Reset\n" );
    }
}
