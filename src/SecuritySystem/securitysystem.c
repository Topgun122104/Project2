#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "gadgets.h"

struct VECTORCLOCK vectorclock;
GADGET *gadget_list[MAX_CONNECTIONS];
int gadget_index = 0;

// Updates the vector clock after it receives a message
void updateVectorClock(char msg[]) {
		
	vectorclock.door = max(vectorclock.door, msg[0] - '0');
	vectorclock.motion = max(vectorclock.motion, msg[2]  - '0');
	vectorclock.keyChain = max(vectorclock.keyChain, msg[4] - '0');
	vectorclock.gateway = max(vectorclock.gateway, msg[6] - '0');
	vectorclock.securitySystem++;
	//vectorclock.securitySystem = max(vectorclock.securitySystem, msg[7] - '0');
	
    char vc[MSG_SIZE];
    sprintf(vc, "Gateway VectorClock:%d-%d-%d-%d-%d,\n",
			vectorclock.door, vectorclock.motion,
			vectorclock.keyChain, vectorclock.gateway,
			vectorclock.securitySystem);
    printf("Updated vector in Gateway is: %s\n", vc);
}

int max(int x, int y) {
	if(x >= y) 
		return x;
	else return y;
}

// Sends the series of unicast messages
void sendMulticast(char *vectorMsg, int s)
{	
	int x;
	for(x=0;x<gadget_index;x++)
	{
		int sock;
		struct sockaddr_in server;
		sock = socket(AF_INET,SOCK_DGRAM, 0);
		
		if (sock == -1)
			puts("couldnt create socket");
				
		GADGET *gadget = gadget_list[x];
		server.sin_addr.s_addr = inet_addr(gadget->ip);
		server.sin_family = AF_INET;
		server.sin_port = htons(gadget->port);
				
		if( sendto(sock, vectorMsg, strlen(vectorMsg), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
			puts("\n send failed");
	}
}

void saveDevices(char string[], char *ip, int port) 
{
	printf("Saving devices\n");
	char *token, *t, *p;
	int x;
	
	t = strtok(string, ",");
	
	for(x=0; x<4; x++) 
	{
		GADGET *gadget = malloc(sizeof(GADGET));
		
		if(NULL != t) 
			gadget->ip = strtok(NULL, ",");
		
		if(NULL != t)
		{
			p = strtok(NULL, ",");
			int port = atoi(p);
			gadget->port = port;
		}
		
		if (port != gadget->port)
			gadget_list[gadget_index++] = gadget;
	}
}

void getCommands(char string[], char **type, char **action)
{
    char *token;

    char *t, *a;

    t = strtok(string, ";");

    if( NULL != t )
        a = strtok(NULL, ";");

    t = strtok(t, ":");

    if( NULL != t )
        *type = strtok(NULL, ":");

    a = strtok(a, ":");

    if( NULL != a )
        *action = strtok(NULL, ":");
}

void deviceListener(void *ptr)
{
	int port = *((int *)ptr);
	int sock;
	struct sockaddr_in server, sender;
	char server_reply[512];
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
	{
		puts("Could not create socket");
	}
	
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	
	bind(sock, (struct sockaddr *)&server, sizeof(server));

	size_t sock_size = sizeof(struct sockaddr_in);
	
	while(1)
	{
		recvfrom(sock, server_reply, sizeof(server_reply), 0, (struct sockaddr *)&sender, (socklen_t *)&sock_size);
		printf("\nReceived multicast msg: %s\n\n", server_reply);
		//TODO UPDATE VECTOR
	}
}

int main(int argc , char *argv[])
{
    FILE *fp = fopen(argv[1],"r");
    FILE *log = fopen(argv[2],"a");

    if ( NULL == log )
    {
        perror("Security System Log File Read Failure");
        return 1;
    }

    if ( NULL == fp )
    {
        perror("Device Configuration File Read Failure");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long pos = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *device = malloc(pos * sizeof(char));
    fread(device, pos, 1, fp);
    fclose(fp);

    char *gateway, *item;

    gateway = strtok(device, "\n");

    if( NULL != gateway )
        item = strtok(NULL, "");

    // Gateway Info Parsing
    char *token, *ip;
    unsigned short int port;

    token = strtok(gateway, ":");

    ip = token;

    if( NULL != token )
    {
        token = strtok(NULL, ":");
    }

    port = (unsigned short int) atoi(token);

    // Device Info Parsing
    char *d_token, *d_token2, *d_token3, *d_type, *d_ip;
    unsigned short int d_port;
    int d_area;

    d_token = strtok(item, ":");

    d_type = d_token;

    if( NULL != d_token )
    {
        d_token = strtok(NULL, "");
    }

    d_token2 = strtok(d_token, ":");

    if( NULL != d_token2 )
    {
        d_ip = d_token2;
        d_token3 = strtok(NULL, "");
    }
    
    d_token3 = strtok(d_token3, ":");

    if( NULL != d_token3 ) 
    {
        d_port = (unsigned short int) atoi(d_token3);
        d_area = atoi(strtok(NULL, ":"));
    }

    int sock;
    struct sockaddr_in server;
    char server_reply[MSG_SIZE];
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    
    server.sin_addr.s_addr = inet_addr( ip );
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
   
    char *type, *action;

    // Register Message
    char msg[MSG_SIZE];
    char log_msg[MSG_SIZE];
    char vc[MSG_SIZE];
    
    sprintf(msg, 
            "Type:register;Action:%s-%s-%d-%d",
            d_type, d_ip, d_port, d_area);
    
    // Current Device State (OFF by default)
    char *state = (char *)malloc(sizeof(char) * 3);
    strcpy(state,OFF);

    // Initilize Vector Clock
    memset(&vectorclock, 0, sizeof(vectorclock));
    vectorclock = (struct VECTORCLOCK){0};
    vectorclock.door = 0;
    vectorclock.motion = 0;
    vectorclock.keyChain = 0;
    vectorclock.gateway = 0;
    vectorclock.securitySystem = 0;
    
    //fcntl(sock, F_SETFL, O_NONBLOCK);
    
    while(1)
    {
        // Send Message to Gateway
        if( send(sock , msg , strlen(msg) , 0) < 0)
        {
            puts("Send failed");
            break;
        }
        
        printf("\nSent: %s\n",msg);
        
        // Send multicast 
        if(gadget_index == 3)
        {
            vectorclock.securitySystem++;

            sprintf(vc,
                	    "Type:vectorClock;Action:%d-%d-%d-%d-%d",
        				vectorclock.door, vectorclock.motion,
        				vectorclock.keyChain, vectorclock.gateway,
        				vectorclock.securitySystem);
    	
            // Send multicast with msg to all devices
           sendMulticast(vc, sock);
           
         	if(send(sock , vc , strlen(vc) , 0) < 0)
         	{
             	puts("Send failed"); 
               break;
            }
            printf("Vector clock message sending is: %s\n", vc);
        }

        // Receive server (gateway) response
        if( recv(sock , server_reply , MSG_SIZE , 0) > 0)
        {
            printf("Gateway reply: %s", server_reply);
            
        	// The device list multicast message
        	if( strncmp( server_reply, "DeviceList", 10) == 0) 
        	{
        		saveDevices(server_reply, d_ip, d_port);
        	}
        
        	else 
        	{
        		printf("\nmade it to else\n");
        		getCommands(server_reply,&type,&action);
        		
        		if ( strncmp( type, CMD_SWITCH, strlen(CMD_SWITCH) ) == 0 )
        		{
        			printf("Device Switching State\n");
        			strcpy(state, action);
        			printf("NEW STATE: %s\n", state);
	
        			sprintf(log_msg, "%d,%s,%s,%u,%s,%d\n",
						sock, d_type, state, (unsigned)time(NULL), d_ip, d_port);
        			fprintf(log, "%s", log_msg);
        			fflush(log);
        		}
			}	
	
			memset(msg, 0, sizeof(msg));
			memset(server_reply, 0, sizeof(server_reply));
			sprintf(msg,
					"Type:%s;Action:%s",
					CMD_STATE, state);
    	
        }
        
        if(gadget_index == 3)
        {
            pthread_t dev1_thread;
            
            if( pthread_create(&dev1_thread, NULL, (void *) &deviceListener, (void *) &d_port) < 0 )
            {
                perror("Thread Creation Failed");
                return 1;
            }
        }
    }
    
    fclose(log); 
    close(sock);
    return 0;
}
