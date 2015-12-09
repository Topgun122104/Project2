#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>

#include "gadgets.h"

struct VECTORCLOCK vectorclock;
GADGET *gadget_list[MAX_CONNECTIONS];
int gadget_index = 0;
int *input;
int array_size; 
FILE *logFile;
char casualty_msg[MSG_SIZE];
unsigned short int cas_port;
int crash_flag = 0;

// Updates the vector clock after it receives a message
void updateVectorClock(char* msg) {
	int d, m, k, g, s;

    	d = atoi(strtok(msg, "-"));

    	m = atoi(strtok(NULL, "-"));

    	k = atoi(strtok(NULL, "-"));

    	g = atoi(strtok(NULL, "-"));

    	s = atoi(strtok(NULL, "-"));
		
	vectorclock.door = max(vectorclock.door, d);
	vectorclock.keyChain++;
	vectorclock.motion = max(vectorclock.motion, m);
	//vectorclock.keyChain = max(vectorclock.keyChain, k);
	vectorclock.gateway = max(vectorclock.gateway, g);
	vectorclock.securitySystem = max(vectorclock.securitySystem, s);
	
    char vc[MSG_SIZE];
    sprintf(vc, "VectorClock:%d-%d-%d-%d-%d,\n",
			vectorclock.door, vectorclock.motion,
			vectorclock.keyChain, vectorclock.gateway,
			vectorclock.securitySystem);
    fprintf(logFile, "%s", vc);
    fflush(logFile);
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
		char *command, *action;
		recvfrom(sock, server_reply, sizeof(server_reply), 0, (struct sockaddr *)&sender, (socklen_t *)&sock_size);
		printf("Received: From:127.0.0.1:%i Msg:%s Time:%u\n\n", port, server_reply, (unsigned)time(NULL));
		getCommands(server_reply, &command, &action);
		updateVectorClock(action);
	}
}

void casualty(void *ptr)
{
	struct sockaddr_in server, sender;
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
	{
		puts("Could not create socket");
	}

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	cas_port += 100;
	server.sin_port = htons(cas_port);

	bind(sock, (struct sockaddr *)&server, sizeof(server));

	size_t sock_size = sizeof(struct sockaddr_in);
	
	while(1)
	{
		int n = 0;
		printf("Listening for First Responder on port: %u\n", cas_port);
		recvfrom(sock, casualty_msg, sizeof(casualty_msg), 0, (struct sockaddr *)&sender, (socklen_t *)&sock_size);
		printf("CASUALTY: %s\n", casualty_msg);
		crash_flag = 1;
		close(sock);
		return;
	}
	
}

int main(int argc , char *argv[])
{
    FILE *fp = fopen(argv[1],"r");

    if ( NULL == fp )
    {
        perror("Sensor Configuration File Read Failure");
        return 1;
    }

    logFile = fopen(argv[3],"a");

    if ( NULL == logFile )
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
	cas_port = (unsigned short int) atoi(s_token3);
        s_area = atoi(strtok(NULL, ","));
    }

    sigignore(SIGPIPE);
    int sock, new_sock;
    struct sockaddr_in server, new_server;
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
    memset(&vectorclock, 0, sizeof(vectorclock));
    vectorclock = (struct VECTORCLOCK){0};
    vectorclock.door = 0;
    vectorclock.motion = 0;
    vectorclock.keyChain = 0;
    vectorclock.gateway = 0;
    vectorclock.securitySystem = 0;


    pthread_t casualty_thread;

     if( pthread_create(&casualty_thread, NULL, (void *) &casualty, NULL) < 0 )
     {
             perror("Thread Creation Failed");
     }
    
    while(1)
    {
	if(crash_flag)
	{
		getCommands(casualty_msg, &type, &action);
		crash_flag = 0;
		puts("Updating IP to alternate GW...");


   		//Parse Current Gateway info
    		token = strtok(action, ",");

    		char* ip_cur = token;

    		if( NULL != token )
    		{
        		token = strtok(NULL, ",");
    		}

    		int port_cur = (unsigned short int) atoi(token);

		if(!strstr(ip_cur, s_ip) || port_cur != s_port)
		{
			//Create socket to slternate gateway
    			new_sock = socket(AF_INET , SOCK_STREAM , 0);
    			if (new_sock == -1)
    			{
        			printf("Could not create new gateway socket");
    			}

    
			new_server.sin_addr.s_addr = inet_addr( ip_cur );
    			new_server.sin_family = AF_INET;
    			new_server.sin_port = htons( port_cur );
			close(sock);
        			
			//Connect to other gateway
    			if (connect(new_sock , (struct sockaddr *)&new_server , sizeof(server)) < 0)
    			{
        			perror("connect failed. Error");
        			return 1;
    			}
	
			//Resend the register to the new gateway
			sock = new_sock;
			if(strncmp( type, CMD_UPDATE, strlen(CMD_UPDATE)) == 0)
			{
				sprintf(msg, "Type:register;Action:%s-%s-%d-%d",
            				s_type, s_ip, s_port, s_area);
				write(sock, msg, strlen(msg));
				memset(msg, 0, sizeof(msg));
			}
			sleep(5);
			continue;
		}
	}
        printf("Send: To:Gateway Msg:%s Time:%u\n\n",msg, (unsigned)time(NULL));
        
        //Send message to gateway
    	if(send(sock , msg , MSG_SIZE , 0) < 0)
    	{
    	 	perror("Send failed"); 
    	}
    	
        // Send multicast 
        if(gadget_index == 3)
        {
            vectorclock.keyChain++;

            sprintf(vc,
                	    "Type:vectorClock;Action:%d-%d-%d-%d-%d",
        				vectorclock.door, vectorclock.motion,
        				vectorclock.keyChain, vectorclock.gateway,
        				vectorclock.securitySystem);
                	
            printf("Send: To:multicast Msg:%s Time:%u\n\n", vc, (unsigned)time(NULL));
                	
            // Send multicast with msg to all devices
            sendMulticast(vc, sock);
                         
            if(send(sock , vc , strlen(vc) , 0) < 0)
            {
             	perror("Send failed\n");
		break; 
            }
        }
        
        fcntl(sock, F_SETFL, O_NONBLOCK);
        
        // Receive server (gateway) response
        if( recv(sock , server_reply , MSG_SIZE , 0) > 0)
        {
            printf("Received:From:gateway Msg:%s Time:%u\n\n", server_reply, (unsigned)time(NULL));

		getCommands(server_reply,&type,&action);
            
        	// The device list multicast message
        	if( strncmp( server_reply, "DeviceList", 10) == 0) 
        	{
        		saveDevices(server_reply, s_ip, s_port);
        	}
		else if (strncmp( type, CMD_UPDATE, strlen(CMD_UPDATE)) == 0 || strncmp(type, CMD_CRASH, strlen(CMD_CRASH)) == 0)
        	{
			puts("Updating IP to alternate GW...");


   			//Parse Current Gateway info
    			token = strtok(action, ",");

    			char* ip_cur = token;

    			if( NULL != token )
    			{
        			token = strtok(NULL, ",");
    			}

    			int port_cur = (unsigned short int) atoi(token);

			if(!strstr(ip_cur, s_ip) || port_cur != s_port)
			{
				//Create socket to slternate gateway
    				new_sock = socket(AF_INET , SOCK_STREAM , 0);
    				if (new_sock == -1)
    				{
        				printf("Could not create new gateway socket");
    				}

    
				new_server.sin_addr.s_addr = inet_addr( ip_cur );
    				new_server.sin_family = AF_INET;
    				new_server.sin_port = htons( port_cur );
				close(sock);
        			
				//Connect to other gateway
    				if (connect(new_sock , (struct sockaddr *)&new_server , sizeof(server)) < 0)
    				{
        				perror("connect failed. Error");
        				return 1;
    				}
	
				//Resend the register to the new gateway
				sock = new_sock;

				sprintf(msg, "Type:register;Action:%s-%s-%d-%d",
            				s_type, s_ip, s_port, s_area);
				write(sock, msg, strlen(msg));
				memset(msg, 0, sizeof(msg));

				puts("IP has been updated...");
				continue;
			}
			else
			{
				puts("Already on correct IP/Port");
			}
        	}
        }
                
        if(gadget_index == 3)
        {
            pthread_t dev1_thread;
            
            if( pthread_create(&dev1_thread, NULL, (void *) &deviceListener, (void *) &s_port) < 0 )
            {
                perror("Thread Creation Failed");
                return 1;
            }
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
		sprintf(log_msg, "%d,%s,%s,%u,%s,%d\n",
						sock, s_type, v, (unsigned)time(NULL), s_ip, s_port);
		fprintf(logFile, "%s", log_msg);
		fflush(logFile);
    }
     
    fclose(logFile);
    close(sock);
    return 0;
}
