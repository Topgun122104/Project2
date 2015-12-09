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
#define CMD_VECTOR "vectorClock"
#define CMD_UPDATE "update"

#define DOOR "Door"
#define MOTION "Motion"
#define KEYCHAIN "Keychain"
#define SECURITYDEVICE "SecurityDevice"
#define DATABASE "Database"

#define PC_MSG "2PC:T"
#define PC_PREPARE "Ready for commit?" 
#define PC_VOTE_READY "Vote:ready"
#define PC_VOTE_ABORT "Vote:abort"
#define PC_MSG_COMMIT "Message:ready:"
#define PC_MSG_ABORT "Message:abort"
#define PC_WAIT_VOTE "Wait:vote"
#define PC_MOV_ABORT "Moving:abort"
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
