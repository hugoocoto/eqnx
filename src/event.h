#ifndef EVENT_H_
#define EVENT_H_ 1

#include "keypress.h"

typedef struct Event Event;

typedef struct Event {
        enum {
                EventKp,
        } code;
        union {
                Keypress kp;
        };
} Event;


#endif
