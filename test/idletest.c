#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IdleMonitor.h"

int initIdleMonitor ( XConfig * xconfig, IdleMonitorConfig * imc ) {

    int ret = 0
      , i = 0
      , major = 0, minor = 0
      , numCounter = 0;

    XSyncValue delta, idletime;

    XSyncSystemCounter * sysCounter= NULL;

    if ( imc == NULL || checkXConfig ( xconfig ) == -1 ) { ret = -1; goto exit; }

    if ( 0 == XSelectInput ( xconfig->dpy
                           , xconfig->root
                           , XSyncAlarmNotifyMask
                           )
       ) { ret = -1; goto exit; }

    if ( 0 == XSyncInitialize ( xconfig->dpy, &major, &minor ) ) {
        ret = -1; goto exit;
    }

    if ( 0 == XSyncQueryExtension ( xconfig->dpy
                                  , &imc->ev_base
                                  , &imc->err_base ) ) {
        ret = -1; goto exit;
    }

    if ( NULL == ( sysCounter
                 = XSyncListSystemCounters ( xconfig->dpy, &numCounter )
                 )
       ) { ret = -1; goto exit; }

    for (i = 0; i < numCounter; i++) {
        if ( 0 == strcmp ( sysCounter[i].name, "IDLETIME" ) ) {
            imc->counter = &sysCounter[i];
        }
    }

    if ( imc->counter == NULL ) { ret = -1; goto exit; }

    XSyncIntToValue (&delta, 0);
    XSyncIntToValue (&idletime, imc->idletime);

    imc->attributes = (XSyncAlarmAttributes *)
                      malloc ( sizeof ( XSyncAlarmAttributes ) );

    imc->attributes->trigger.counter    = imc->counter->counter;
    imc->attributes->trigger.value_type = XSyncAbsolute;
    imc->attributes->trigger.test_type  = XSyncPositiveComparison;
    imc->attributes->trigger.wait_value = idletime;
    imc->attributes->delta              = delta;

exit:
    if ( sysCounter != NULL ) XSyncFreeSystemCounterList ( sysCounter );
    return ret;
}

void finalizeIdleMonitor ( IdleMonitorConfig * imc ) {
    if ( imc != NULL && imc->attributes != NULL ) {
        free ( imc->attributes );
    }
}

int runIdleMonitor ( XConfig           * xconfig
                   , IdleMonitorConfig * imc
                   , SignalEmitter     * se
                   ) {

    int ret = 0;

    unsigned long flags = XSyncCACounter
                        | XSyncCAValueType
                        | XSyncCATestType
                        | XSyncCAValue
                        | XSyncCADelta
                        ;

    int idlecount = 0;
    XEvent xEvent;
    Time lastEventTime;
    XSyncValue idletime;
    XSyncAlarmNotifyEvent * alarmEvent = NULL;

    if ( checkIdleMonitorConfig ( imc ) == -1
      || checkXConfig ( xconfig ) == -1
       ) { ret = -1; goto exit; }

    XSyncAlarm alarm = XSyncCreateAlarm ( xconfig->dpy, flags, imc->attributes );

    while ( 1 ) {
        XNextEvent ( xconfig->dpy, &xEvent );

        if ( xEvent.type != imc->ev_base + XSyncAlarmNotify ) continue;

        alarmEvent = (XSyncAlarmNotifyEvent *) &xEvent;

        if ( XSyncValueLessThan ( alarmEvent->counter_value
                                , alarmEvent->alarm_value
                                )
           ) {
            imc->attributes->trigger.test_type  = XSyncPositiveComparison;
            
            if ( alarmEvent->time - lastEventTime < imc->backoff ) {
                XSyncIntToValue ( &idletime
                                , imc->strategy->getBackoff ( &imc->idletime
                                                            , &idlecount
                                                            )
                                );
                imc->attributes->trigger.wait_value = idletime;
            } else {
                XSyncIntToValue ( &idletime, imc->idletime );
                imc->attributes->trigger.wait_value = idletime;
                idlecount = 0;
            }

            if ( lastEventTime != alarmEvent->time ) {
                se->emitSignal ( se, "Reset", NULL );
                fprintf ( stderr, "Reset; reaction_time: %lu; idlecount: %i; idletime: %i\n"
                        , alarmEvent->time - lastEventTime
                        , idlecount
                        , imc->strategy->getBackoff ( &imc->idletime
                                                    , &idlecount
                                                    )
                        );
            }
        } else {
            imc->attributes->trigger.test_type = XSyncNegativeComparison;
            if ( lastEventTime != alarmEvent->time ) {
                idlecount++;
                se->emitSignal ( se, "Idle", NULL );
                fprintf ( stderr, "Idle\n" );
            }
        }

        XSyncChangeAlarm ( xconfig->dpy, alarm, flags, imc->attributes );
        lastEventTime = alarmEvent->time;

    }

exit:
    return ret;

}

int checkIdleMonitorConfig ( IdleMonitorConfig * imc ) {
    if ( imc == NULL ) {
        return -1;
    } else if ( imc->counter == NULL || imc->attributes == NULL ) {
        return -1;
    } else {
        return 0;
    }
}
