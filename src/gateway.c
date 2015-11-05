#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include<pthread.h>
#include "utils.h"

// Gadget's Thread
void *connection(void *socket_desc)
{
    int client_desc = * (int *) socket_desc;

    DEVICE *device = malloc(sizeof(DEVICE));
    
    device->id = client_desc;

    int read_size;

    char client_msg[BUFSIZE], msg[BUFSIZE];

    // Receive Data from Client
    while( (read_size = recv(client_desc, client_msg, BUFSIZE, 0)) > 0 )
    {
        memset(msg, 0, sizeof(msg));
        printf("\nFROM CLIENT:\n%s\n\n",client_msg);

    }

    if( read_size == 0 ) 
    {
        char msg[100];
        sprintf(msg, 
                "Client %d Unregistered...", 
                client_desc);
        puts(msg);
        fflush(stdout);
    } 
    
    else if ( read_size == -1 ) 
    {
        perror("Data Read Failed!\n");
    }

    return 0;
}

/**
* Main method to confirm arguments are correct, and read in ConfigFile.
*/
int main (int argc, char *argv[]) {

	if(argc != 2) {
		//Assumes argv[0] is the program name
		perror("Must provide a configuration file!\n");
		return 1;
    	} else {

		//Assumes argv[1] is a filename to open
		FILE *configFile = fopen(argv[1], "r");

		//fopen returns 0, the NULL pointer, on failure
		if(configFile == 0) {
			printf("Could not open config file!\n");
			return 1;
		}


		GATEWAY* gw = malloc(sizeof(GATEWAY)); 
		//Get the first line from input file
		char line1[BUFSIZE];
		fgets(line1, BUFSIZE-1, configFile);
		char *token;
    		unsigned short int port;
		
		token = strtok(line1, ":");
 		gw->ip = token;
    		if( NULL != token )
    		{
        		token = strtok(NULL, ":");
    		}

    		gw->port = (unsigned short int) atoi(token);

    		fclose(configFile);

		printf("IP: %s \nPort:%d \n", gw->ip, gw->port);

		int socket_desc, client_desc, c;
    		struct sockaddr_in server, client;

    		//Create socket
    		socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    		if(socket_desc == -1) {
    			perror("Gateway could not create socket!\n");
			return 1;
    		}

    		//Prepare the sockaddr_in structure
    		server.sin_family = AF_INET;
    		server.sin_addr.s_addr = INADDR_ANY;
    		server.sin_port = htons(gw->port);

    		//Bind
    		if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
			//Print error message
			perror("Socket bind failed!\n");
			return 1;
    		}

		puts("Bind...\n");
    
    		//Listen
    		listen(socket_desc, 5);

		puts("Listening...");

    		//Accept any incoming connection
    		c = sizeof(struct sockaddr_in);
		pthread_t thread;

    		// Complete Connection with a Client
    		while( client_desc = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *) &c) )
    		{
        		if( client_desc < 0 ) 
        		{
            			perror("Connection Failed!\n");
            			return 1;
        		}

        		puts("Connection Accepted...");

        		if( pthread_create( &thread, NULL, connection, (void*) &client_desc) < 0 )
        		{
            			perror("Thread Creation Failed");
            			return 1;
        		}
    		}
    	}

    	return 0;
}
