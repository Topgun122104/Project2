#ifndef UTILS_H
#define UTILS_H

#define ON  "on"
#define OFF "off"

#define MSG_SIZE 100
#define MAX_CONNECTIONS 100
#define BUFSIZE 1025

#define INTERVAL 5

typedef struct GATEWAY{
	char* ip;
	int port;
}GATEWAY;

typedef struct DEVICE{
	int id;
	char* ip;
	int port;
	char* type;
	char* state;
}DEVICE;

#endif
