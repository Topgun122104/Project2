#ifndef UTILS_H
#define UTILS_H

#define ON  "on"
#define OFF "off"
#define OPEN "Open"
#define CLOSED "Close"
#define TRU "True"
#define FALS "False"

#define MSG_SIZE 100
#define MAX_CONNECTIONS 100
#define BUFSIZE 1025

#define CMD_STATE    "currState"
#define CMD_REGISTER "register"
#define CMD_INSERT "insert"
#define CMD_VALUE "currValue"
#define CMD_SWITCH "switch"

#define DOOR "Door"
#define MOTION "Motion"
#define KEYCHAIN "Keychain"
#define SECURITYDEVICE "SecurityDevice"
#define DATABASE "Database"

#define CONFIRM "Device Registration Confirmed"

#define INTERVAL 5

typedef struct gadget {
    char *ip;
    char *state;
    char *gadgetType;
    int id;
    int area;
    int port;
    int interval;
    int currValue;
} GADGET;

typedef struct devicelist {
	char *doorIP;
	char *motionIP;
	char *keychainIP;
	char *securitydeviceIP;
	int doorPort;
	int motionPort;
	int keychainPort;
	int securitydevicePort;
} DEVICELIST;

typedef struct DOORSTATE{
	int time;
	char* state;
}DOORSTATE;

typedef struct VECTORCLOCK {
	int door;
	int motion;
	int keyChain;
	int gateway;
	int securitySystem;
} VECTORCLOCK;

#endif
