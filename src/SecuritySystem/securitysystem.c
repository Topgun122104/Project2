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
FILE *logFile;

// Updates the vector clock after it receives a message
void updateVectorClock(char msg[]) {
	int d, m, k, g, s;

    	d = atoi(strtok(msg, "-"));

    	m = atoi(strtok(NULL, "-"));

    	k = atoi(strtok(NULL, "-"));

    	g = atoi(strtok(NULL, "-"));

    	s = atoi(strtok(NULL, "-"));
		
	vectorclock.door = max(vectorclock.door, d);
	vectorclock.securitySystem++;
	vectorclock.motion = max(vectorclock.motion, m);
	vectorclock.keyChain = max(vectorclock.keyChain, k);
	vectorclock.gateway = max(vectorclock.gateway, g);
	//vectorclock.securitySystem = max(vectorclock.securitySystem, s);
	
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
		server.sin_addr.s_addr = inet_addr("127.0.0.1");
		server.sin_family = AF_INET;
		server.sin_port = htons(gadget->port);
				
		if( sendto(sock, vectorMsg, strlen(vectorMsg), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
			puts("\n send failed!");
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

int main(int argc , char *argv[])
{
    FILE *fp = fopen(argv[1],"r");
    logFile = fopen(argv[2],"a");

    if ( NULL == logFile )
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

    int sock, sock2;
    struct sockaddr_in server, server2;
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
    
    //Create socket (sock2 is mulitcast socket)
    sock2 = socket(AF_INET , SOCK_STREAM , 0);
    if (sock2 == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    
    server2.sin_addr.s_addr = inet_addr( ip );
    server2.sin_family = AF_INET;
    server2.sin_port = htons( 6000 );
 
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
        
        printf("Send: To:Gateway Msg:%s Time:%u\n\n",msg, (unsigned)time(NULL));
        
        // Send multicast 
        if(gadget_index == 3)
        {
            vectorclock.securitySystem++;

            sprintf(vc,
                	    "Type:vectorClock;Action:%d-%d-%d-%d-%d",
        				vectorclock.door, vectorclock.motion,
        				vectorclock.keyChain, vectorclock.gateway,
        				vectorclock.securitySystem);
            printf("Send: To:multicast Msg:%s Time:%u\n\n", vc, (unsigned)time(NULL));
            // Send multicast with msg to all devices
           sendMulticast(vc, sock2);
           
         	if(send(sock , vc , strlen(vc) , 0) < 0)
         	{
             	puts("Send failed2"); 
               break;
            }
        }

        // Receive server (gateway) response
        if( recv(sock , server_reply , MSG_SIZE , 0) > 0)
        {
            printf("Received:From:gateway Msg:%s Time:%u\n\nf", server_reply, (unsigned)time(NULL));
            
        	// The device list multicast message
        	if( strncmp( server_reply, "DeviceList", 10) == 0) 
        	{
        		saveDevices(server_reply, d_ip, d_port);
        	}
        
        	else 
        	{
        		getCommands(server_reply,&type,&action);
        		
        		if ( strncmp( type, CMD_SWITCH, strlen(CMD_SWITCH) ) == 0 )
        		{
        			strcpy(state, action);
	
        			sprintf(log_msg, "%d,%s,%s,%u,%s,%d\n",
						sock, d_type, state, (unsigned)time(NULL), d_ip, d_port);
        			fprintf(logFile, "%s", log_msg);
        			fflush(logFile);
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
				if(!strstr(ip_cur, d_ip) || port_cur != d_port)
				{
					int new_sock; 
					struct sockaddr_in new_server;
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
					//Resend the register to the new gateway only if load blanacing
					if(strncmp( type, CMD_UPDATE, strlen(CMD_UPDATE)) == 0)
					{
						sprintf(msg, "Type:register;Action:%s-%s-%d-%d",
            						d_type, d_ip, d_port, d_area);
					}
					
					sock = new_sock;
					puts("IP has been updated...");
					memset(msg, 0, sizeof(msg));
					memset(server_reply, 0, sizeof(server_reply));
					continue;
        			}
				else
				{
					puts("Already on correct IP/Port");
				}
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
    
    fclose(logFile); 
    close(sock);
    return 0;
}
