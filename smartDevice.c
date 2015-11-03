#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE 100
int on = 0; //initially device is off
int registered = 0; //initially not registered
int sock;
struct sockaddr_in server;

/**
 * Sends a message to the gateway saying that the device state has changed.
 * (Either the device was turned on or off).
 */
void sendCurrState() {
	char str1[] = "Type:currState;Action:";
	char strOn[3] = "on";
	char strOff[4] = "off";
	char currMessage[1000];

	//Message saying the device is off
	if (on == 0) {
		strcpy(currMessage, str1);
		strncat(currMessage, strOff, 3);
	}

	//Message saying the device is on
	else {
		strcpy(currMessage, str1);
		strncat(currMessage, strOn, 3);
	}

	//Send currState message to gateway. Print message if it's not sent.
	if(send (sock, currMessage, strlen(currMessage), 0) <0) {
		puts("recv failed");
	}
}

/**
 * Initalizes the socket connection between the gateway and the device.
 * If the device has not registered yet, then the device sends a register message to the gateway.
 * If the device receives a message from the gateway to turn the device on/off, then it updates the
 *    state, and sends a currState message back to the gateway.
 */
void initSocket(char *ip, char *p, char *deviceIP, char *devicePort, char *areaID) {
    int port = atoi(p);
    char message[1000], server_reply[2000];

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1) {
        printf("Could not create socket");
    }
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    //Connect to gateway (server)
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
		perror("Connection failed. Error");
    }

    //Continually listening/sending messages to gateway
    while(1) {

    	//If the device hasn't registered yet, send register message to gateway.
	    if(registered == 0) {

			char str1[] = "Type:register;Action:device-";
			char str2[] = "-";

			strcpy(message, str1);
			strcat(message, deviceIP);
			strcat(message, str2);
			strcat(message, devicePort);
			strcat(message, str2);
			strcat(message, areaID);
			strcat(message, "\n");

			//Send register message to gateway
			if(send (sock, message, strlen(message), 0) <0) {
				puts("recv failed");
			}
			registered = 1; //device is now registered.
	    } else {

	        //Check if the device received a message from gateway
	        if(recv(sock , server_reply , 2000 , 0) > 0) {
	            char *type, *a, *action;
	            type = strtok(server_reply, ";");

	            //Verify the message is a switch message
	            if(strncmp(type, "Type:Switch", 8) == 0) {
	    			a = strtok(NULL, ":");
	    			action = strtok(NULL, "\n");

	    			//Send currState message to gateway
	    			if(strncmp(action, "on", 2) == 0) {
	    				on = 1;
	    			} else {
	    				on = 0;
	    			}
	    			sendCurrState();
	            }
	        }
		}
    }
}

int main (int argc, char *argv[]) {
    if(argc != 2) {

	//Assumes argv[0] is the program name
	printf("usage: %s configFileName.txt\n", argv[0]);
    } else {

		//Assumes argv[1] is a filename to open
		FILE *configFile = fopen(argv[1], "r");

		//fopen returns 0, the NULL pointer, on failure
		if(configFile == 0) {
			printf("Could not open file: %s\n", argv[1]);
		} else {

			//Get the first two lines
			char line1[BUFSIZE];
			char line2[BUFSIZE];
	
			fgets(line1, BUFSIZE-1, configFile);
			fgets(line2, BUFSIZE-1, configFile);

			//Parsing 1st line:
			char *gatewayIP, *gatewayPort;
			gatewayIP = strtok(line1, ":");
			gatewayPort = strtok(NULL, " ,");

			//Parsing 2nd line:
			char *device, *deviceIP, *devicePort, *deviceID;
			device = strtok(line2, ":");
			deviceIP = strtok(NULL, ":");
			devicePort = strtok(NULL, ":");
			deviceID = strtok(NULL, " ,");

			//Initalize connection.
			initSocket(gatewayIP, gatewayPort, deviceIP, devicePort, deviceID);
		}
    }
}

