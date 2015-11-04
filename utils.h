#ifndef UTILS_H
#define UTILS_H

#define ON  "on"
#define OFF "off"

#define MSG_SIZE 100
#define MAX_CONNECTIONS 100

#define INTERVAL 5

typedef struct gadget {
    char *ip;
    char *state;
    char *gadgetType;
    int id;
    int port;
    int area;
    int currValue;
    int interval;
} GADGET;

#endif
