#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE 100
int doorState = 1; 		//initially door is open (doorState=0 means door is closed, doorState=1 means door is open)
int registered = 0; 	//initially not registered
int sock;
int currentValue;
struct sockaddr_in server;


int sendcurrValueMessage(char *msg) {
	send (sock, msg, strlen(msg), 0);
	return 1;
}

/**
 * Sends a message to the gateway saying that the sensor state has changed.
 * (Either the door was opened or closed).
 */
int sendCurrState() {
	char str1[] = "Type:currState;Action:";
	char strOpen[5] = "Open";
	char strClose[6] = "Close";
	char currMessage[1000];

	//Message saying the sensor door is closed
	if (doorState == 0) {
		strcpy(currMessage, str1);
		strncat(currMessage, strClose, 3);
	}

	//Message saying the sensor door is open
	else {
		strcpy(currMessage, str1);
		strncat(currMessage, strOpen, 3);
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

    		//For the doorSensor, only send currState when the
    		// 	door has changed open/close state
    		for(i=0; i<arraySize; i++) {
    			sleep(1);
    			if(arraySize[i] != doorState) {
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
							doorState = 1;
						} else {
							doorState = 0;
						}
						int n = sendCurrState();
						continue;

					}
				   continue;
				} continue;
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
		int t1; 			//The time when door state changes
		int currState;		//The state of the door (open=1, closed=0)
		int numLines = 0; 	//Used to count number of lines in the file
		char *state;		//The state of the door (open or closed)
		int i = 0; 			//Counter

		//Get the number of lines in the file
		while(fgets(buff, BUFSIZE-1, inputFile) !=NULL) {
			numLines = numLines +1;
		}
		fclose(inputFile);

		//Array to store times that the door state will change
		int updateTimesArray[numLines];

		//Store the times that door state changes
		FILE *inputFile2 = fopen(argv[2], "r");
		while(fgets(buff, BUFSIZE-1, inputFile2) !=NULL) {
			t1 = atoi(strtok(buff, ";"));
			state = strtok(NULL, " ,");

			updateTimesArray[i] = t1;
			i = i+1;
		}
		fclose(inputFile2);

		//Stores if door is open (1) or closed(0)
		int finalValues[t1+2];

		//Loop again, storing values in array
		int j = 0;
		FILE *inputFile3 = fopen(argv[2], "r");
		while(fgets(buff, BUFSIZE-1, inputFile3) !=NULL) {
			t1 = atoi(strtok(buff, ";"));
			state = strtok(NULL, " ,");

			if(strncmp(state, "Open", 4) == 0) {
				printf("state for %d = %s\n", t1, state);
				currState=1;
			} else {
				currState=0;
			}

			//Counter used to iterate through arrValues array
			if(j+1 == numLines) {
				finalValues[t1+1] = currState;
				printf("Storing finalValues[%d] = %d\n", t1, currState);
				printf("REACHED END\n");
			} else {
				j = j+1;
				for(t1; t1<updateTimesArray[j]; t1++) {
					finalValues[t1] = currState;
					printf("Storing finalValues[%d] = %d   less than: %i\n", t1, currState, updateTimesArray[j]);
				}
			}
		}
		fclose(inputFile3);
		int k;
		  /* output each array element's value */
		for (k = 0; k < t1+1; k++) {
		      printf("finalValues[%d] = %d\n", k, finalValues[k] );
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
		return initSocket(gatewayIP, gatewayPort, sensorIP, sensorPort, sensorID, finalValues, t1+1);
	}
    }
}

