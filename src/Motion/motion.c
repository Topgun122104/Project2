#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#include "gadgets.h"

struct VECTORCLOCK vectorclock;
GADGET *gadget_list[MAX_CONNECTIONS];
int gadget_index = 0;
int *input;
int array_size; 

// Updates the vector clock after it receives a message
void updateVectorClock(char msg[]) {
		
	vectorclock.door = max(vectorclock.door, msg[0] - '0');
	vectorclock.motion++;
	//vectorclock.motion = max(vectorclock.motion, msg[2]  - '0');
	vectorclock.keyChain = max(vectorclock.keyChain, msg[4] - '0');
	vectorclock.gateway = max(vectorclock.gateway, msg[6] - '0');
	vectorclock.securitySystem = max(vectorclock.securitySystem, msg[7] - '0');
	
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
void sendMulticast(char msg[], int sock)
{	
	printf("Trying to send multicast\n");

	// Send remainder of multicast messages
	int x;
	for(x=0;x<gadget_index;x++)
	{   
		GADGET *gadget = gadget_list[x];
		struct sockaddr_in addrDest;
		
		printf("SENDINT TO IP: %s\b", gadget->ip);
		addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
		addrDest.sin_family = AF_INET;
		addrDest.sin_port = htons(gadget->port);
		
	    //Connect to remote server
	    if (connect(sock , (struct sockaddr *)&addrDest , sizeof(addrDest)) < 0)
	    {
	        perror("Connect failed. Error");
	        break;
	    }
	    
		if( sendto(sock, msg , strlen(msg) , 0, 
				(struct sockaddr *)&addrDest, sizeof(addrDest)) < 0)
		{
			puts("Send failed");
			break;
		} 
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

char* toString(int s)
{
	char *str = (char*)malloc(sizeof(char) * 6);
	if(s)
	{
		strcpy(str, "True");
	}
	else
	{
		strcpy(str, "False");
	}

	return str;
}

int isOn(char *state)
{
    return ( strncmp(state, ON, strlen(ON)) == 0 ) ? 1 : 0;
}

void *timer()
{
    FILE *fp = fopen("../SampleConfigurationFiles/SampleMotionInput.txt","r");

    if ( NULL == fp )
    {
        perror("Motion Input File Read Failure");
    }

    // Get the last line
    char *last;
    size_t last_len = 0;
    
    while(!feof(fp))
        getline(&last, &last_len, fp);

    fseek(fp, 0, SEEK_SET);

    char *last_tok;

    last_tok = strtok(last, ",");
    
    if( NULL != last_tok )
        array_size = atoi(strtok(NULL, ","));    

    char *temp, *temper;
    size_t line_len = 0;
    int startTime, endTime, val;

    input = malloc((array_size + 1) * sizeof(int));
    
    while( getline(&temp, &line_len, fp) > 0 )
    {
        char *token, *token2;

        token = strtok(temp, ",");

        startTime = atoi(token);

        if( NULL != token )
            token2 = strtok(NULL, ",");

        endTime = atoi(token2);

        if( NULL != token2 )
            temper = strtok(NULL, "");

        //Convert True or False to 1 or 0 respectively
		if(strstr(temper, TRU))
		{
			val = 1;
		}
		else
		{
			val = 0;
		}

        int i;
        for(i=startTime; i<=endTime; i++)
            input[i] = val;
    }
    
    fclose(fp);

    int x;
    for(x=0;x<array_size;x++)
        printf("%d",input[x]);
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

int main(int argc , char *argv[])
{
    FILE *fp = fopen(argv[1],"r");

    if ( NULL == fp )
    {
        perror("Sensor Configuration File Read Failure");
        return 1;
    }

    FILE *log = fopen(argv[3],"a");

    if ( NULL == fp )
    {
        perror("Motion Sensor Log File Read Failure");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long pos = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *sensor = malloc(pos * sizeof(char));
    fread(sensor, pos, 1, fp);
    fclose(fp);

    char *gateway, *item;

    gateway = strtok(sensor, "\n");

    if( NULL != gateway )
        item = strtok(NULL, "");

    // Gateway Info Parsing
    char *token, *ip;
    unsigned short int port;

    token = strtok(gateway, ",");

    ip = token;

    if( NULL != token )
    {
        token = strtok(NULL, ",");
    }

    port = (unsigned short int) atoi(token);
    printf("PORT IS : %i\n\n\n", port);

    // Device Info Parsing
    char *s_token, *s_token2, *s_token3, *s_type, *s_ip;
    unsigned short int s_port;
    int s_area;

    s_token = strtok(item, ",");

    s_type = s_token;

    if( NULL != s_token )
        s_token = strtok(NULL, "");

    s_token2 = strtok(s_token, ",");

    if( NULL != s_token2 )
    {
        s_ip = s_token2;
        s_token3 = strtok(NULL, "");
    }

    s_token3 = strtok(s_token3, ",");

    if( NULL != s_token3 )
    {
        s_port = (unsigned short int) atoi(s_token3);
        s_area = atoi(strtok(NULL, ","));
    }

    int sock;
    int multiSock;
    
    struct sockaddr_in server;
    struct sockaddr_in addrDest;
    
    char *message;
    char server_reply[MSG_SIZE];
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    printf("Socket created: %i\n", sock);
    
    //Create socket
    multiSock = socket(AF_INET , SOCK_STREAM , 0);
    if (multiSock == -1)
    {
        printf("Could not create socket");
    }
    printf("Socket2  created: %d\n", multiSock);
     
    server.sin_addr.s_addr = inet_addr( ip );
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
    
	addrDest.sin_addr.s_addr = inet_addr( ip );
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(5997);
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");

    // Time Thread
    pthread_t time_thread;

    if( pthread_create(&time_thread, NULL, timer, (void *) NULL) < 0 )
    {
        perror("Thread Creation Failed");
        return 1;
    }

    // Wait interval (in seconds)
    int interval = 5;
    int state_interval = 0;

    // Sensor State (On by default)
    char *state = ON;

    char *type, *action;

    int currValue = 0;
    
    // Register Message
    char msg[MSG_SIZE];
    char log_msg[MSG_SIZE];
    char vc[MSG_SIZE];

    sprintf(msg,
            "Type:register;Action:%s-%s-%d-%d",
            s_type, s_ip, s_port, s_area);
    
    // Initilize Vector Clock
    vectorclock.door = 0;
    vectorclock.motion = 0;
    vectorclock.keyChain = 0;
    vectorclock.gateway = 0;
    vectorclock.securitySystem = 0;

    fcntl(sock, F_SETFL, O_NONBLOCK);
    fcntl(multiSock, F_SETFL, O_NONBLOCK);
    
    while(1)
    {
        printf("\nSend to Gateway: %s\n",msg);
                
        //Send message to gateway
      	if(send(sock , msg , strlen(msg) , 0) < 0)
      	{
          	puts("Send failed"); 
            break;
        }
                
        memset(vc, 0, sizeof(vc));

        // Send multicast 
        if(gadget_index == 3)
        {
        	printf("Updating vector clock!\n");
            vectorclock.motion++;

            sprintf(vc,
                	    "Type:vectorClock;Action:%d-%d-%d-%d-%d",
        				vectorclock.door, vectorclock.motion,
        				vectorclock.keyChain, vectorclock.gateway,
        				vectorclock.securitySystem);
                	
             printf("Vector clock message sending is: %s\n", vc);
                	
            // Send multicast with msg to all devices
           //sendMulticast(vc, multiSock);
           
         	if(send(sock , vc , strlen(vc) , 0) < 0)
         	{
             	puts("Send failed"); 
               break;
           }
        }
        
        // Receive server (gateway) response
        if( recv(sock , server_reply , MSG_SIZE , 0) > 0)
        {
            puts("Gateway reply:");
            puts(server_reply);
            
        	// The device list multicast message
        	if( strncmp( server_reply, "DeviceList", 10) == 0) 
        	{
        		saveDevices(server_reply, s_ip, s_port);
        	}
        }
        
        // Receive multicast messages from other devices ?
        if( recv(multiSock, server_reply, MSG_SIZE, 0) > 0)
        {
        		updateVectorClock(server_reply);
        }
        
        // Wait for time interval (5 seconds default)
        sleep(interval);

        state_interval += interval;

        if(state_interval > array_size)
            state_interval -= array_size;

        currValue = input[state_interval];

        memset(msg, 0, sizeof(msg));

        if( isOn(state) )
        {            
		sprintf(msg,
                    "Type:currValue;Action:%d",
                    currValue);
        }

	char* v = toString(currValue);
	sprintf(log_msg, "Log Msg: %d,%s,%s,%u,%s,%d\n",
                    sock, s_type, v, (unsigned)time(NULL), s_ip, s_port);
	fprintf(log, "%s", log_msg);
	fflush(log);
    }
     
    fclose(log);
    close(sock);
    return 0;
}
