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
	char *temp, *t;

	temp = strtok(msg, ",");
	
	if(NULL != temp) // door
	{
		t = strtok(NULL, ",");
		int d = atoi(t);
		vectorclock.door = max(vectorclock.door, d);
	}
	
	if(NULL != temp)
	{
		t = strtok(NULL, ",");
		vectorclock.motion = max(vectorclock.motion, atoi(t));
	}
	
	t = strtok(temp, ",");
	vectorclock.keyChain++;
	
	if(NULL != temp)
	{
		t = strtok(NULL, ",");
		vectorclock.gateway = max(vectorclock.gateway, atoi(t));
	}
	
	if(NULL != temp)
	{
		t = strtok(temp, ",");
		vectorclock.securitySystem = max(vectorclock.securitySystem, atoi(t));
	}
	
    char vc[MSG_SIZE];
    sprintf(vc, "Keychain VectorClock:%d,%d,%d,%d,%d,\n",
			vectorclock.door, vectorclock.motion,
			vectorclock.keyChain, vectorclock.gateway,
			vectorclock.securitySystem);
}

int max(int x, int y) {
	if(x >= y) 
		return x;
	else return y;
}

// Sends the series of unicast messages
void sendMulticast(char msg[], int sock)
{	
	// Send remainder of multicast messages
	int x;
	for(x=0;x<gadget_index;x++)
	{   
		GADGET *gadget = gadget_list[x];
		struct sockaddr_in addrDest;
		
		addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
		addrDest.sin_family = AF_INET;
		addrDest.sin_port = htons(gadget->port);
		
		if( sendto(sock, msg , strlen(msg) , 0, 
				(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
		{
			puts("Send failed");
			break;
		} 
		
	}
	
    // Send Gateway Current Monitored Value
	if(send(sock , msg , strlen(msg) , 0) < 0)
	{
	 	puts("Send failed"); 
	 	//break;
	 	//TODO this used to have a break statement, maybe return -1 if break? and
	 	//  main actually breaks?
	}
}

void saveDevices(char string[], char *ip, int port) 
{
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
		
		if (strncmp(ip, gadget->ip, strlen(ip)) != 0)
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
    FILE *fp = fopen("../SampleConfigurationFiles/SampleKeychainInput.txt","r");

    if ( NULL == fp )
    {
        perror("Keychain Input File Read Failure");
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
    {
        printf("%d",input[x]);
    }
    printf("\n");
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
        perror("Keychain Sensor Log File Read Failure");
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
    struct sockaddr_in server;
    //char message[1000];
    char *message;
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
    
    while(1)
    {
        printf("\nSend to Gateway: %s\n",msg);
        
        // Send multicast to all devices
        sendMulticast(msg, sock);
        
        // Receive multicast messages from other devices
        if( recv(sock , server_reply , MSG_SIZE , 0) > 0)
        {
            puts("Gateway reply:");
            puts(server_reply);
            
        	// The device list multicast message
        	if( strncmp( server_reply, "DeviceList", 10) == 0)
        	{
        		saveDevices(server_reply, s_ip, s_port);
        	}
        	
        	// Receiving multicast message from other devices
        	else
        	{
        		updateVectorClock(server_reply);
        		
        	}
        }

        // Wait for time interval (5 seconds default)
        sleep(interval);

        state_interval += interval;

        if(state_interval > array_size)
            state_interval -= array_size;

        currValue = input[state_interval];
        
        memset(vc, 0, sizeof(vc));
        memset(msg, 0, sizeof(msg));

        // Increment local vector clock, only after gadgets registered
    	if(gadget_index != 0) {
            vectorclock.keyChain++;
        
            sprintf(vc, "VectorClock,%d,%d,%d,%d,%d,",
    				vectorclock.door, vectorclock.motion,
    				vectorclock.keyChain, vectorclock.gateway,
    				vectorclock.securitySystem);
            
            if( isOn(state) )
            {            
            	sprintf(msg,
                        "%sType:currValue;Action:%d",
                        vc, currValue);
            }
    	}
    	
    	else {

			if( isOn(state) )
			{            
				sprintf(msg,
						"Type:currValue;Action:%d",
						currValue);
			}
    	}
    	
		char* v = toString(currValue);
		sprintf(log_msg, "%d,%s,%s,%u,%s,%d\n",
						sock, s_type, v, (unsigned)time(NULL), s_ip, s_port);
		fprintf(log, "%s", log_msg);
		fflush(log);
    }
     
    fclose(log);
    close(sock);
    return 0;
}
