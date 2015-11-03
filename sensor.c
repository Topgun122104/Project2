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
int currentValue;
struct sockaddr_in server;
int interval = 5;


int sendcurrValueMessage(char *msg) {
	send (sock, msg, strlen(msg), 0);
	return 1;
}

/**
 * Sends a message to the gateway saying that the sensor state has changed.
 * (Either the device was turned on or off).
 */
int sendCurrState() {
	char str1[] = "Type:currState;Action:";
	char strOn[3] = "on";
	char strOff[4] = "off";
	char currMessage[1000];

	//Message saying the sensor is off
	if (on == 0) {
		strcpy(currMessage, str1);
		strncat(currMessage, strOff, 3);
	}

	//Message saying the sensor is on
	else {
		strcpy(currMessage, str1);
		strncat(currMessage, strOn, 3);
	}

	//Send currState message to gateway. Print message if it's not sent.
	if(send (sock, currMessage, strlen(currMessage), 0) <0) {
		puts("recv failed");
		return 1;
	} else {
		return 1;
	}
}

int initSocket(char *ip, char *p, char *sensorIP, char *sensorPort, char *areaID, int arrayValues[], int arraySize) {
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
		return 1;
    }

    //If the device hasn't registered yet, send register message to gateway.
    if (registered == 0) {

        char str1[] = "Type:register;Action:sensor-";
        char str2[] = "-";

		strcpy(message, str1);
		strcat(message, sensorIP);
		strcat(message, str2);
		strcat(message, sensorPort);
		strcat(message, str2);
		strcat(message, areaID);
		strcat(message, "\n");

		//Send register message to gateway
		if(send (sock, message, strlen(message), 0) <0) {
			puts("recv failed");
		}
		registered = 1; //device is now registered.
    }

    while (1) {
    	while(1) {
    		int i;

    		for(i=0; i<arraySize; i++) {
    			sleep(1);
    			if ((i % interval) == 0) {
    				char *msg;
    				char output[30];

					msg = "Type:currValue;Action:";
					strcpy(output, "Type:currValue;Action:");

					int val = arrayValues[i];
					char value[3];

					sprintf(value, "%d", val);
					strcat(output, value);

					int n = sendcurrValueMessage(output);

					continue;
				}
    			continue;

				//Check if the device received a message from gateway
				if (recv(sock , server_reply , 2000 , 0) > 0) {
				   char *type, *a, *action, *setInt;
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
						int n = sendCurrState();
						continue;

					} else if (strncmp(type, "Type:setInterval", 8) == 0) {
						a = strtok(NULL, ":");
						setInt = strtok(NULL, "\n");
						interval = atoi(setInt);
						continue;
					}
				   continue;
				} continue;

			continue;
            }
        }
    }
}

int main (int argc, char *argv[]) {
    if(argc != 3) {

	//Assumes argv[0] is the program name
	printf("usage: %s configFileName.txt inputFile.txt\n", argv[0]);
    } else {

	//Assumes argv[1] is a filename to open
	FILE *configFile = fopen(argv[1], "r");
    FILE *inputFile = fopen(argv[2], "r");
	
	if(inputFile == 0 || configFile == 0) {
	    printf("Could not open file: %s\n", argv[2]);
	} else {
		char buff[BUFSIZE];
		int t1, t2, val;
		//Find largest value
		while(fgets(buff, BUFSIZE-1, inputFile) !=NULL) {

			t1 = atoi(strtok(buff, ";"));
			t2 = atoi(strtok(NULL, ";"));
			val = atoi(strtok(NULL, " ,"));
		}

		fclose(inputFile);
		int arrValues[t2];
		int finalValues[t2];

		FILE *inputFile = fopen(argv[2], "r");

		//Loop again, storing values in array
		while(fgets(buff, BUFSIZE-1, inputFile) !=NULL) {
			t1 = atoi(strtok(buff, ";"));
			t2 = atoi(strtok(NULL, ";"));
			val = atoi(strtok(NULL, " ,"));

			for(t1; t1<t2; t1++) {
				arrValues[t1] = val;
			}
		}

		int i = 0;
		for(i; i<t2; i++) {
			finalValues[i] = arrValues[i];
		}
	
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
	    char *sensor, *sensorIP, *sensorPort, *sensorID;
	    sensor = strtok(line2, ":");
	    sensorIP = strtok(NULL, ":");
	    sensorPort = strtok(NULL, ":");
	    sensorID = strtok(NULL, " ,");

	    //Initalize connection.
		return initSocket(gatewayIP, gatewayPort, sensorIP, sensorPort, sensorID, finalValues, t2);
	}
    }
}

