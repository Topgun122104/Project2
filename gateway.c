#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include<pthread.h>

#define BUFSIZE 100
int deviceID, sensorID, itemID, lineNum;
int areaID = 0;
int deviceOn = 0; //off is 0, on is 1
void *connection_handler(void *);
int deviceArray[100];
int deviceItemArray[100];
int deviceLastValue[100];

char deviceIP[16];
int deviceID, devicePort;

/**
 * Initializes the socket. Will create a new thread for new items connected.
 */
int initSocket(char *ip, char *p) {

    int socket_desc, new_socket, c, port, *new_sock;
    struct sockaddr_in server, client;
    port = atoi(p);

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1) {
    	printf("Gateway could not create socket");
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    //Bind
    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
		//Print error message
		perror("bind failed. Error");
		return 1;
    }
    
    //Listen
    listen(socket_desc, 3);

    //Accept any incoming connection
    c = sizeof(struct sockaddr_in);

    //Accept connection from an incoming device or sensor, and create new thread
    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
    	pthread_t thread;
    	new_sock = malloc(1);
    	*new_sock = new_socket;

    	//Create thread
    	if(pthread_create( &thread, NULL, connection_handler, (void*) new_sock) < 0)  {
    		perror("Could not create thread");
    		return 1;
    	}
    }

    if(new_socket < 0) {
		perror("accept failed");
		return 1;
    }

    return 0;
}

/**
 * Handles connections for each device/sensor.
 */
void *connection_handler(void *socket_desc) {

	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char client_message[2000];

    //Receive a message from client
    while( (read_size = recv(sock, client_message, 2000, 0)) > 0) {

		//Parse the message, see if register event
		char *type, *a, *action, *clientIP, *port, *aID, *state;
		char actionType[10]; //sets the final action, used for all messaging
		char ip[16];
		int id, p, sock_desc_device, sock_desc_sensor, value, thisID;
		int dashCount = 0;

		type = strtok(client_message, ";");

		//If the messge type is register
		if (strncmp(type, "Type:register", 8) == 0) {
			lineNum = 0; //ADDED
			a = strtok(NULL, ":");
			action = strtok(NULL, "-");
			clientIP = strtok(NULL, "-");
			port = strtok(NULL, "-");
			aID = strtok(NULL, "\0");
			strcpy(actionType, action);
			strcpy(ip, clientIP);

			id = atoi(aID);
			p = atoi(port);

			itemID++;
			thisID = itemID;
			lineNum++;

			//If the area IDs are the same, do not print new line.
			//If the area IDs are the same, just set the sock.
			if(id == areaID) {
				if(strncmp("sensor", action, 6) == 0) {
					sock_desc_sensor = sock;
				} else if (strncmp(action,"device",6)==0) {
					deviceID = thisID;
					deviceArray[areaID] = sock;
					deviceItemArray[areaID] = itemID;
					deviceLastValue[areaID] = 0;
					sock_desc_device = sock;
				}
			} else {
				areaID = id;
				if(strncmp("sensor", action, 6) == 0) {
					sock_desc_sensor = sock;
					//sock_desc_device = 0;
				} else if (strncmp(action,"device",6) == 0 ) {
					deviceID = thisID;
					deviceArray[areaID] = sock;
					deviceItemArray[areaID] = itemID;
					deviceLastValue[areaID] = 0;
					sock_desc_device = sock;
					sock_desc_sensor = 0;
				}
			}

			if(strncmp(action,"device",6)==0) {
				deviceOn = 0;
				deviceID = itemID;
				strcpy(deviceIP, ip);
				devicePort = p;
				dashCount++;

				printf("-----------------------------------------------------\n");
				printf("Line %i:%i----%s:%i----%s----%i----off\n", lineNum, deviceID, deviceIP, devicePort, action, id);
			}
			memset(&client_message[0], 0, sizeof(client_message));
		}

		//If the message type is currState, print out the current state of the itemtype
		if(strncmp(type, "Type:currState", 10) == 0) {
			a = strtok(NULL, ":");
			state = strtok(NULL, "  \0");

			// Only change state if it's a device
			if(thisID == deviceID) {
				if(strncmp(state, "off", 3) == 0) {
					state = "off";
					deviceOn--;
				} else if (strncmp(state, "on", 2) == 0) {
					state = "on";
					deviceOn++;
				}
			}

			memset(&client_message[0], 0, sizeof(client_message));
		}

		//If the message type is currValue, record if the currValue changed or if the state changed.
		if(strncmp(type, "Type:currValue", 8) ==0) {

			char *val;
			int currValue;
			a = strtok(NULL, ":");
			val = strtok(NULL, " \0");
			value = atoi(val) + 0;
			currValue = value;
			char *currState;

			//Turn the device on if value is below 32
			if((currValue < 32) && (deviceOn == 0) && currValue != 0) {

				//Send to device sock, turning device on
				char *testMessage = "Type:Switch;Action:on";
				int n;
				n = send(deviceArray[id], testMessage, strlen(testMessage), 0);

			}

			//Turn the device off if value is above 34
			else if ((currValue > 34) && (deviceOn == 1)) {

				//Send to device sock, turning device off
				char *testMessage = "Type:Switch;Action:off";
				int n;
				n = send(deviceArray[id], testMessage, strlen(testMessage), 0);
			}

			if(deviceOn == 1) {
				currState = "on";
			} else {
				currState = "off";
			}

			//If the value changed
			if(deviceLastValue[id] != value) {
				if(dashCount == 0) {
					dashCount++;
					lineNum = 1;
					printf("-----------------------------------------------------\n");
					printf("Line %i:%i----%s:%i----device----%i----%s\n", lineNum, deviceItemArray[id], deviceIP, devicePort, id, currState);
				}
			}

			deviceLastValue[id] = value;
			lineNum++;
			printf("Line %i:%i----%s:%i----%s----%i----%i\n", lineNum, thisID, ip, p, actionType, id, value);
			memset(&client_message[0], 0, sizeof(client_message));


		}
    }

    if(read_size == 0) {
		//puts("Client disconnected");
		fflush(stdout);
    } else if(read_size == -1) {
    	perror("recv failed");
    	//return 1;
    }

    printf("-----------------------------------------------------\n");
    return 0;
}

/**
 * Main method to confirm arguments are correct, and read in ConfigFile.
 */
int main (int argc, char *argv[]) {

	if(argc != 2) {

		//Assumes argv[0] is the program name
		printf("usage: %s configFileName.txt inputFile.txt\n", argv[0]);
    } else {

		//Assumes argv[1] is a filename to open
		FILE *configFile = fopen(argv[1], "r");

		//fopen returns 0, the NULL pointer, on failure
		if(configFile == 0) {
			printf("Could not open file: %s\n", argv[1]);
		} else {

			// Initialize the counters used for line number and item id
			itemID = 0;
			sensorID = 0;
			deviceID = 0;
			lineNum = 0;

			//Get the first line from input file
			char line1[BUFSIZE];
			fgets(line1, BUFSIZE-1, configFile);

			//Parsing 1st line:
			char *gatewayIP, *gatewayPort;
			gatewayIP = strtok(line1, ":");
			gatewayPort = strtok(NULL, " ,");
			printf("IP: %s Port:%s \n", gatewayIP, gatewayPort);

			//Call initialize method
			return initSocket(gatewayIP, gatewayPort);
		}
    }
}

