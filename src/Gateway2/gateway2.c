#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#include "gadgets.h"

// List of Devices and Sensors
GADGET *gadget_list[MAX_CONNECTIONS];
GADGET *global_list[MAX_CONNECTIONS];
struct VECTORCLOCK vectorclock;
int gadget_index = 0;
int global_index = 0;
int db_sock = -1;
FILE *logFile;
int currDoor = 0;
//Last time door was open
unsigned int doorTime = -1;
int currMotion = 0;
//Last time Motion was detected
unsigned int motionTime = -1;
int currKeychain = 0;
//Last time keychain was detected
unsigned int keychainTime = -1;
int currSystem = 0;
char* gw_sec_ip = NULL;
unsigned short int gw_sec_port = 0;
char* gw_pri_ip = NULL;
unsigned short int gw_pri_port = 0;
int gw_index = 1;
int amPri = 0;
int sock_pri;
int sock_sec;
struct sockaddr_in pri_skt;
int num_gws = 2;

// Updates the vector clock after it receives a message
void updateVectorClock(char* msg) {
	int d, m, k, g, s;

    	d = atoi(strtok(msg, "-"));

    	m = atoi(strtok(NULL, "-"));

    	k = atoi(strtok(NULL, "-"));

    	g = atoi(strtok(NULL, "-"));

    	s = atoi(strtok(NULL, "-"));
		
	vectorclock.door = max(vectorclock.door, d);
	vectorclock.gateway++;
	vectorclock.motion = max(vectorclock.motion, m);
	vectorclock.keyChain = max(vectorclock.keyChain, k);
	//vectorclock.gateway = max(vectorclock.gateway, g);
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

// Determines if intruder entered.
// Intruder entered if 1st: Door is open, then motion sensed with no keychain present
// Called when motion sensed
int ifIntruder() {
	int result = 0;
	//Someone entered without the keychain #AboutToHaveABadDay
	if(currDoor && (doorTime < motionTime) && !currKeychain)
	{
		result = 1;
	}
	return result;
}

int userHome()
{
	int result = 0; 
	if(currDoor && (doorTime < motionTime) && currKeychain)
	{
		result = 1;
	}

	return result;
}

int homeEmpty()
{
	int result = 0; 
	if(currDoor && (motionTime < doorTime))
	{
		result = 1;
	}

	return result;
}

char* toString(int s, char* type)
{
	char *str = (char*)malloc(sizeof(char) * 6);
	if(strstr(type, MOTION) || strstr(type, KEYCHAIN))
	{
		if(s)
		{
			strcpy(str, "True");
		}
		else
		{
			strcpy(str, "False");
		}
	}
	else if(strstr(type, DOOR))
	{
		if(s)
		{
			strcpy(str, "Open");
		}
		else
		{
			strcpy(str, "Close");
		}
	}

	return str;
}

char* generateDBMsg(int id, char* type, char* sta, int val, char* ip, int port)
{
	unsigned int stamp = (unsigned)time(NULL);
	char tempVal[6];
	if(strstr(type, MOTION))
	{
		if(val)
		{
			strcpy(tempVal, "True");
			motionTime = stamp;
			currMotion = 1;
		}
		else
		{
			strcpy(tempVal, "False");
			currMotion = 1;
		}
	}
	else if(strstr(type, KEYCHAIN))
	{
		if(val)
		{
			strcpy(tempVal, "True");
			keychainTime = stamp;
			currKeychain = 1;
		}
		else
		{
			strcpy(tempVal, "False");
			currKeychain = 0;
		}
	}
	else if(strstr(type, DOOR))
	{
		if(val)
		{
			strcpy(tempVal, "Open");
			doorTime = stamp;
			currDoor = 1;
		}
		else
		{
			strcpy(tempVal, "Close");
			currDoor = 0;
		}
	}

	char *str = (char*)malloc(sizeof(char) * 100);
	if(strstr(type, SECURITYDEVICE))
	{
		sprintf(str, "Type:insert;Action:%d,%s,%s,%u,%s,%d\n",
                    id, type, sta, stamp, ip, port);
	}
	else
	{
		sprintf(str, "Type:insert;Action:%d,%s,%s,%u,%s,%d\n",
                    id, type, tempVal, stamp, ip, port);
	}

	return str;
}

// Send a copy of the message received from device to other gateway
void forwardMessage(char string[])
{
	char msg2[100];
	//If it is the primary gateway, then send to sec
	if(amPri == 1)
	{
		sock_sec = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
    		struct sockaddr_in server_sec;
		int size = sizeof(server_sec);

		puts("Socket created");
	     
	    	server_sec.sin_addr.s_addr = inet_addr( gw_sec_ip );
	    	server_sec.sin_family = AF_INET;
	    	server_sec.sin_port = htons( gw_sec_port );

		sprintf(msg2, "MSG FROM PRIMARY!!! %s", string);

		sendto(sock_sec, msg2, strlen(msg2), 0, (struct sockaddr*) &server_sec, size);
		printf("SENT to secondary: %s\n", msg2);
		close(sock_sec);
	}
	
	//Else, send to primary
	else
	{
		sock_pri = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
    		struct sockaddr_in server_pri;
		int size = sizeof(server_pri);

		puts("Socket created");
	     
	    	server_pri.sin_addr.s_addr = inet_addr( gw_pri_ip );
	    	server_pri.sin_family = AF_INET;
	    	server_pri.sin_port = htons( gw_pri_port );

		sprintf(msg2, "MSG FROM SECONDARY!!! %s", string);

		sendto(sock_pri, msg2, strlen(string), 0, (struct sockaddr*) &server_pri, size);
		printf("SENT to primary: %s\n", msg2);
		close(sock_pri);
	}	
}

// Send multicast to all devices, except database, with list of messages
void sendDeviceListMulticast() 
{
	char deviceList[MSG_SIZE];
	int x;
	int index = 0;
	if(num_gws == 2)
	{
		index = gadget_index;
	}
	else
	{
		index = global_index;
	}
	strcpy(deviceList, "DeviceList");
	
	// Creating the device list to send to all devices
    for(x=0; x<index; x++)
    {
	GADGET *gadget;
	if(num_gws == 2)
	{
		gadget = gadget_list[x];
	}
	else
	{
		gadget = global_list[x];
	}
        char comma[2] = ",";
        char portChar[10];
        char line[20];        
        
        if(strcmp( gadget->gadgetType, DATABASE) != 0) 
        {
			sprintf(portChar, "%d", gadget->port);
			sprintf(line, ",%s,%d", gadget->ip, gadget->port);
			strcat(deviceList, line);

        }
        deviceList[MSG_SIZE] = '\0';
    }
        
    //Sending unicast for all devices with the deviceList
    for(x=0; x<index; x++)
    {
	GADGET *gadget;
	if(num_gws == 2)
	{
		gadget = gadget_list[x];
	}
	else
	{
		gadget = global_list[x];
	}
        
        if(strcmp (gadget-> gadgetType, DATABASE) != 0)
        {
			struct sockaddr_in addrDest;
	
			addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
			addrDest.sin_family = AF_INET;
			addrDest.sin_port = htons(gadget->port);
			
			if( sendto(gadget->id, deviceList , strlen(deviceList) , 0, 
					(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
			{
				puts("Send failed");
				break;
			} 
        }
    }
    printf("Send: To:multicast Msg:%s Time:%u\n\n", deviceList, (unsigned)time(NULL));
}

// Print the latest information about Devices and Sensors
void printGadgets()
{
	
    printf("------------------------------------------------\n");

    if(num_gws == 2)
    {
	int x;
  	for(x=0; x<gadget_index; x++)
    	{
        	GADGET *gadget = gadget_list[x];
		char msg[MSG_SIZE];

		if(!strstr(gadget->gadgetType, DATABASE))
		{
        		char* currStr = toString(gadget->currValue, gadget->gadgetType);
			//If Security Device, we want to display current state instead of current value
			if(strstr(gadget->gadgetType, SECURITYDEVICE))
			{
        			sprintf(msg, "%d----%s----%s----%u----%s----%d",
                	  		gadget->id, gadget->gadgetType, gadget->state, (unsigned)time(NULL), gadget->ip, gadget->port);
			}
			else
			{
				sprintf(msg, "%d----%s----%s----%u----%s----%d",
                 		  	gadget->id, gadget->gadgetType, currStr, (unsigned)time(NULL), gadget->ip, gadget->port);
			}

		}
		puts(msg); 
         }
    }
    else
    {
	int x;
    	for(x=0; x<global_index; x++)
    	{
        	GADGET *gadget = global_list[x];
		char msg[MSG_SIZE];

		if(!strstr(gadget->gadgetType, DATABASE))
		{
        		char* currStr = toString(gadget->currValue, gadget->gadgetType);
			//If Security Device, we want to display current state instead of current value
			if(strstr(gadget->gadgetType, SECURITYDEVICE))
			{
        			sprintf(msg, "%d----%s----%s----%u----%s----%d",
                	  		gadget->id, gadget->gadgetType, gadget->state, (unsigned)time(NULL), gadget->ip, gadget->port);
			}
			else
			{
				sprintf(msg, "%d----%s----%s----%u----%s----%d",
                 		        gadget->id, gadget->gadgetType, currStr, (unsigned)time(NULL), gadget->ip, gadget->port);
			}

		}
		puts(msg);
         } 
    }
         
    puts("------------------------------------------------");
}

// Get the Type and Action from a received command
void getCommands(char string[], char **type, char **action)
{
    char *tok, *tok2, *tok3, *tok4, *vector;

    tok = strtok(string, ";");

    if( NULL != tok )
        tok2 = strtok(NULL, ";");
    
    if( NULL != tok2 )
    	tok3 = strtok(NULL, ";");
    
    tok = strtok(tok, ":");

    if( NULL != tok )
        *type = strtok(NULL, ":");

    tok2 = strtok(tok2, ":");

    if( NULL != tok2 )
        *action = strtok(NULL, ":");
    
    if( NULL != tok3 )
    	tok3 = strtok(tok3, ":");
    
    if( NULL != tok3 ) {
    	vector = strtok(NULL, ":");
    	updateVectorClock(vector);
    } 
}

// Get the Information of a Device or Sensor
void getInfo(char string[], char **gadgetType, char **ip, int *port, int *area)
{
    char *tok, *tok2, *a_tok, *c_tok;

    tok = strtok(string, "-");

    *gadgetType = tok;

    if( NULL != tok )
        tok2 = strtok(NULL, "");

    *ip = strtok(tok2, "-");

    if( NULL != ip )
        a_tok = strtok(NULL, "");

    c_tok = strtok(a_tok, "-");

    *port = atoi(c_tok); 
    
    if( NULL != c_tok )
        *area = atoi(strtok(NULL, "-"));
}

int isOn(char *state)
{
    return ( strncmp(state, ON, strlen(ON)) == 0 ) ? 1 : 0;
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
		printf("OMG IT WORKED: From:127.0.0.1:%i Msg:%s Time:%u\n\n", port, server_reply, (unsigned)time(NULL));

	}
}

void first_responder()
{
	//Set all gadget is local list to NULL
	int y;
        for(y=0; y<gadget_index; y++)
        {
		gadget_list[y] = NULL;
	}

	//puts("Gadget List cancelled...");
	char up_msg[MSG_SIZE];
	if(amPri)
	{
		sprintf(up_msg, "Type:update;Action:%s,%u", gw_pri_ip, gw_pri_port);
	}
	else
	{
		sprintf(up_msg, "%s,%u", gw_pri_ip, gw_pri_port);
	}

	//Update all devices in global list with alive GW IP
	int x;
	//Global Index will get larger as things register
	int global_temp = global_index;
        for(x=0; x<global_temp; x++)
        {
        	GADGET *gadget = global_list[x];
		
                if(strcmp (gadget-> gadgetType, DATABASE) != 0)
       		{
			printf("Sending update message to %s\n", gadget->gadgetType);
			struct sockaddr_in addrDest;
	
			addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
			addrDest.sin_family = AF_INET;
			addrDest.sin_port = htons(gadget->port);
			if( sendto(gadget->id, up_msg , strlen(up_msg) , 0, 
					(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
			{
				puts("Send failed");
				break;
			} 
       		}
        }
	puts("First Responder Triage Complete...");
}

void heartbeat_send(void *ptr)
{
	//puts("Inside HeartBeat Send...");
	struct sockaddr_in server, sender;
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
	{
		puts("Could not create socket");
	}
	
	if(amPri)
	{
		//puts("Primary Heartbeat...");
		server.sin_addr.s_addr = inet_addr(gw_sec_ip);
		server.sin_family = AF_INET;
		server.sin_port = htons(gw_sec_port);
	}
	else
	{
		//puts("Secondary Heartbeat...");
		server.sin_addr.s_addr = inet_addr(gw_pri_ip);
		server.sin_family = AF_INET;
		server.sin_port = htons(gw_pri_port);
	}

	
	bind(sock, (struct sockaddr *)&server, sizeof(server));

	size_t sock_size = sizeof(struct sockaddr_in);
	char* heart_msg = "ALIVE";
	
	while(1)
	{
		//One gateway has failed and heartbeats are no longer necessary
		if(num_gws < 2)
		{
			puts("Stopping HeartBeat...");
			puts("Calling First Responder...");
			//Alive GW takes over everything
			first_responder();
			return;
		}
		//Send a heartbeat status every 5 
		//puts("Sending Heartbeat...");		
		int n = 0;
		n = sendto(sock, heart_msg, strlen(heart_msg), 0, (struct sockaddr*) &server, sock_size);

		//puts("Heartbeat sent...");
		sleep(3);
	}
}

void heartbeat_recv(void *ptr)
{
	//puts("Inside HeartBeat Recv...");
	struct sockaddr_in server, sender;
	struct timeval timeout;
	timeout.tv_sec = 6;
	timeout.tv_usec = 0;
	char server_reply[512];
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
	{
		puts("Could not create socket");
	}
	
	if(amPri)
	{
		//puts("Primary Heartbeat...");
		server.sin_addr.s_addr = inet_addr(gw_pri_ip);
		server.sin_family = AF_INET;
		server.sin_port = htons(gw_pri_port);
	}
	else
	{
		//puts("Secondary Heartbeat...");
		server.sin_addr.s_addr = inet_addr(gw_sec_ip);
		server.sin_family = AF_INET;
		server.sin_port = htons(gw_sec_port);
	}

	
	bind(sock, (struct sockaddr *)&server, sizeof(server));
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0)
	{
		perror("Socket Timeout Failed!");
	}

	size_t sock_size = sizeof(struct sockaddr_in);
	
	while(1)
	{
		//puts("Receiving Heartbeat...");
		int n = 0;
		n = recvfrom(sock, server_reply, sizeof(server_reply), 0, (struct sockaddr *)&sender, (socklen_t *)&sock_size);
		if(n <= 0)
		{
			puts("Gateway has failed!!");
			num_gws--;
			return;
		}
		
		//printf("HEARTBEAT: %s\n", server_reply);
		
		memset(server_reply, 0, sizeof(server_reply));

	}
	close(sock);
}

// Gadget's Thread
void *connection(void *skt_desc)
{
    int client_skt_desc = * (int *) skt_desc;

    GADGET *gadget = malloc(sizeof(GADGET));
    
    gadget->id = client_skt_desc;

    int read_size;
    char *command, *action;

    char client_msg[MSG_SIZE], msg[MSG_SIZE], out_msg[MSG_SIZE], cpy_msg[MSG_SIZE], sec_msg[MSG_SIZE];
    char* log_msg;
    char vc[MSG_SIZE];

    socklen_t size = sizeof(pri_skt);
    char buf[MSG_SIZE];
    int recv_len;

    pthread_t dev1_thread, dev2_thread;
    if(client_skt_desc == sock_pri)
    {
            if( pthread_create(&dev1_thread, NULL, (void *) &deviceListener, (void *) &gw_pri_port) < 0 )
            {
                perror("Thread Creation Failed");
            }
    } 
    else if( client_skt_desc == sock_sec) 
    {
            if( pthread_create(&dev2_thread, NULL, (void *) &deviceListener, (void *) &gw_sec_port) < 0 )
            {
                perror("Thread Creation Failed");
            }
    }
    // Receive Data from Client
    while( (read_size = recv(client_skt_desc, client_msg, sizeof(client_msg), 0)) > 0 )
    {
        printf("Received: Msg:%s Time:%u\n\n",client_msg, (unsigned)time(NULL));
        memset(msg, 0, sizeof(msg));
        memset(cpy_msg, 0, sizeof(cpy_msg));
        memset(sec_msg, 0, sizeof(sec_msg));
	memset(buf, 0, sizeof(buf));
        strncpy(cpy_msg, client_msg, sizeof(client_msg));
        
        getCommands(client_msg, &command, &action);

        // Register Case
        if( strncmp(command, CMD_REGISTER, strlen(CMD_REGISTER)) == 0 )
        {
	    //This gateway is the coordinator
	    if(amPri)
	    {
		puts("Inside Primary Loop");
		//Device stays
		if(gw_index % 2 != 0 || num_gws < 2)
		{
			puts("Device stays at this gateway!");
            		char *t_gadgetType, *t_ip;
            
            		getInfo(action, &t_gadgetType, &t_ip, &gadget->port, &gadget->area);
            
            		gadget->gadgetType = (char *)malloc(sizeof(char) * strlen(t_gadgetType));
            		memcpy(gadget->gadgetType, t_gadgetType, strlen(t_gadgetType));

            		gadget->ip = (char *)malloc(sizeof(char) * strlen(t_ip));
            		memcpy(gadget->ip, t_ip, strlen(t_ip));

	    		//Security System is off by default
            		if(strstr(gadget->gadgetType, SECURITYDEVICE))
	    		{
				gadget->state = (char *)malloc(sizeof(char) * 4);            
            		        strcpy(gadget->state, OFF);
	    		}
	    		else
	    		{
				gadget->state = (char *)malloc(sizeof(char) * 3);            
           		        strcpy(gadget->state, ON);
	   		}
			
			if(num_gws == 2)
			{
				gadget_list[gadget_index++] = gadget;
			}
			
			global_list[global_index++] = gadget;
			puts("Device added to *local* and global list...");

            		// Creating list of the ports and ips to send to all devices 

			if(num_gws == 2)
			{
            			if (gadget_index == 5) {
             				sendDeviceListMulticast();
            			}
			}
            		else
			{
				if (global_index == 5) {
             				sendDeviceListMulticast();
            			}
			}
            
           		 printGadgets();
            
	   		 if(strstr(gadget->gadgetType, DATABASE))
	   		 {
				db_sock = client_skt_desc;
	   		 }

			 gw_index++;
		 }
		 //Device Goes
		 else if(num_gws == 2)
	         {	
			//Create Gadget and add to global list
			char *t_gadgetType, *t_ip;
            
            		getInfo(action, &t_gadgetType, &t_ip, &gadget->port, &gadget->area);
            
            		gadget->gadgetType = (char *)malloc(sizeof(char) * strlen(t_gadgetType));
            		memcpy(gadget->gadgetType, t_gadgetType, strlen(t_gadgetType));

            		gadget->ip = (char *)malloc(sizeof(char) * strlen(t_ip));
            		memcpy(gadget->ip, t_ip, strlen(t_ip));

	    		//Security System is off by default
            		if(strstr(gadget->gadgetType, SECURITYDEVICE))
	    		{
				gadget->state = (char *)malloc(sizeof(char) * 4);            
            		        strcpy(gadget->state, OFF);
	    		}
	    		else
	    		{
				gadget->state = (char *)malloc(sizeof(char) * 3);            
           		        strcpy(gadget->state, ON);
	   		}

            		global_list[global_index++] = gadget;
 			//Send update message to switch to alternate GW
			puts("Device  added to global and is being reassigned!");
			char switch_msg[MSG_SIZE];
			sprintf(switch_msg, "Type:update;Action:%s,%u", gw_sec_ip, gw_sec_port);
			printf("Switch Message: %s\n", switch_msg);
			write(client_skt_desc, switch_msg, strlen(switch_msg));

			gw_index++;
			return;
	   	 }
	    }
	    //This gateway is not the coordinator
	    else
	    {
		 char *t_gadgetType, *t_ip;
            
            	 getInfo(action, &t_gadgetType, &t_ip, &gadget->port, &gadget->area);
            
            	 gadget->gadgetType = (char *)malloc(sizeof(char) * strlen(t_gadgetType));
            	 memcpy(gadget->gadgetType, t_gadgetType, strlen(t_gadgetType));

            	 gadget->ip = (char *)malloc(sizeof(char) * strlen(t_ip));
            	 memcpy(gadget->ip, t_ip, strlen(t_ip));

	    	//Security System is off by default
            	if(strstr(gadget->gadgetType, SECURITYDEVICE))
	    	{
			gadget->state = (char *)malloc(sizeof(char) * 4);            
            	        strcpy(gadget->state, OFF);
	    	}
	    	else
	    	{
			gadget->state = (char *)malloc(sizeof(char) * 3);            
           	        strcpy(gadget->state, ON);
	   	}

            	gadget_list[gadget_index++] = gadget;
		puts("Device added to local list...");

            	// Creating list of the ports and ips to send to all devices            

            	if (gadget_index == 5) {
             	   sendDeviceListMulticast();
            	}
            
           	 printGadgets();
            
	   	 if(strstr(gadget->gadgetType, DATABASE))
	   	 {
			db_sock = client_skt_desc;
	   	 }

	    }
        }

        // Current Temperature Value Case
        else if( strncmp(command, CMD_VALUE, strlen(CMD_VALUE)) == 0 ) 
        {
            if( atoi(action) != gadget->currValue )
            {
                gadget->currValue = atoi(action);
                printGadgets();
		forwardMessage(cpy_msg);
            }
            
			log_msg = generateDBMsg(client_skt_desc, gadget->gadgetType, gadget->state, gadget->currValue, gadget->ip, 						gadget->port);
			fprintf(logFile, "%s", log_msg);
			fflush(logFile);
			write(db_sock, log_msg, strlen(log_msg));
        }

        // Current State Case
        else if( strncmp( command, CMD_STATE, strlen(CMD_STATE) ) == 0 && 
                 strncmp( gadget->state, action, strlen(action) ) != 0 )
        {	
            memset(gadget->state, 0, 3);
            if( strncmp (action, "offType", strlen("offType")) == 0)
            {

            	strcpy(gadget->state, "off");
            }
            else if( strncmp (action, "onType", strlen("onType")) == 0)
            {
            	strcpy(gadget->state, "on");
            }
            else 
            {
            	strcpy(gadget->state, action);
            }
            
	    printGadgets();
	    forwardMessage(cpy_msg);
	    log_msg = generateDBMsg(client_skt_desc, gadget->gadgetType, gadget->state, gadget->currValue, gadget->ip, 					gadget->port);
	    fprintf(logFile, "%s", log_msg);
	    fflush(logFile);
	    write(db_sock, log_msg, strlen(log_msg));
        }
        
        // vectorClock state case
        else if ( strncmp( command, CMD_VECTOR, strlen(CMD_VECTOR) ) == 0) 
        {    		
    		sprintf(log_msg, "Gateway VectorClock:%d-%d-%d-%d-%d",
			vectorclock.door, vectorclock.motion,
			vectorclock.keyChain, vectorclock.gateway,
			vectorclock.securitySystem); 

		    fprintf(logFile, "%s\n", log_msg);
		    fflush(logFile);
		    write(db_sock, log_msg, strlen(log_msg));
    		
        }
        
        // Check if user entered
        if(strncmp( gadget->gadgetType, MOTION, strlen(MOTION) ) == 0 && 
           strncmp( toString(gadget->currValue, gadget->gadgetType), TRU, strlen(TRU) ) == 0) 
        {        	
        	//First check for intruder
		if(ifIntruder())
		{
			printf("Intruder Alert! Time: %u\n\n", (unsigned)time(NULL));
			char* gw_log_msg = "ALARM SOUNDED!!\n";
			char* db_log_msg = "Type:insert;Action:ALARM SOUNDED!\n";
			fprintf(logFile, "%s %u", gw_log_msg, (unsigned)time(NULL));
			fflush(logFile);
			write(db_sock, db_log_msg, strlen(db_log_msg));
		}
        	else if(userHome()) 
        	{
			printf("User Home! Time: %u\n\n", (unsigned)time(NULL));
        		sprintf(sec_msg, "Type:switch;Action:off");
        		// Send turn on message to security system
        		int x;
			if(num_gws == 2)
			{
        			for(x=0; x<gadget_index; x++)
        			{
        			        GADGET *gadget = gadget_list[x];
        			        
        		 	       if(strcmp (gadget-> gadgetType, SECURITYDEVICE) == 0)
        		 	       {
        					struct sockaddr_in addrDest;
        				
        					addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
        					addrDest.sin_family = AF_INET;
        					addrDest.sin_port = htons(gadget->port);
        					
        					if( sendto(gadget->id, sec_msg , strlen(sec_msg) , 0, 
        						(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
        					{
        						puts("Send failed");
        						break;
        					} 
        		      		        printf("Send: To: securitydevice Msg:%s Time:%u\n\n", sec_msg, (unsigned)time(NULL));

        		       	       }
        			}
			}
			else
			{
				for(x=0; x<global_index; x++)
        			{
        			        GADGET *gadget = global_list[x];
        			        
        		 	       if(strcmp (gadget-> gadgetType, SECURITYDEVICE) == 0)
        		 	       {
        					struct sockaddr_in addrDest;
        				
        					addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
        					addrDest.sin_family = AF_INET;
        					addrDest.sin_port = htons(gadget->port);
        					
        					if( sendto(gadget->id, sec_msg , strlen(sec_msg) , 0, 
        						(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
        					{
        						puts("Send failed");
        						break;
        					} 
        		      		        printf("Send: To: securitydevice Msg:%s Time:%u\n\n", sec_msg, (unsigned)time(NULL));

        		       	       }
        			}
			}
	    
        		char* gw_log_msg = "User Came Home\n";
			char* db_log_msg = "Type:insert;Action:User Came Home!\n";
			fprintf(logFile, "%s %u", gw_log_msg, (unsigned)time(NULL));
			fflush(logFile);
			write(db_sock, db_log_msg, strlen(db_log_msg));
        	}
        }
        
        // Check if user left
        if( strncmp( gadget->gadgetType, DOOR, strlen(DOOR) ) == 0 &&
            strncmp( toString(gadget->currValue, gadget->gadgetType), OPEN, strlen(OPEN) ) == 0) 
        {
        	if(userHome()) 
        	{
			printf("Notice: User Home Time: %u\n\n",(unsigned)time(NULL));
          		sprintf(sec_msg, "Type:switch;Action:off");
        	        //Send turn on message to security system
           		int x;
			if(num_gws == 2)
			{
        			for(x=0; x<gadget_index; x++)
        			{
        			        GADGET *gadget = gadget_list[x];
        			        
        		 	       if(strcmp (gadget-> gadgetType, SECURITYDEVICE) == 0)
        		 	       {
        					struct sockaddr_in addrDest;
        				
        					addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
        					addrDest.sin_family = AF_INET;
        					addrDest.sin_port = htons(gadget->port);
        					
        					if( sendto(gadget->id, sec_msg , strlen(sec_msg) , 0, 
        						(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
        					{
        						puts("Send failed");
        						break;
        					} 
        		      		        printf("Send: To: securitydevice Msg:%s Time:%u\n\n", sec_msg, (unsigned)time(NULL));

        		       	       }
        			}
			}
			else
			{
				for(x=0; x<global_index; x++)
        			{
        			        GADGET *gadget = global_list[x];
        			        
        		 	       if(strcmp (gadget-> gadgetType, SECURITYDEVICE) == 0)
        		 	       {
        					struct sockaddr_in addrDest;
        				
        					addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
        					addrDest.sin_family = AF_INET;
        					addrDest.sin_port = htons(gadget->port);
        					
        					if( sendto(gadget->id, sec_msg , strlen(sec_msg) , 0, 
        						(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
        					{
        						puts("Send failed");
        						break;
        					} 
        		      		        printf("Send: To: securitydevice Msg:%s Time:%u\n\n", sec_msg, (unsigned)time(NULL));

        		       	       }
        			}
			}
        	        char* gw_log_msg = "User Came Home\n";
		        char* db_log_msg = "Type:insert;Action:User Came Home!\n";
		        fprintf(logFile, "%s %u", gw_log_msg, (unsigned)time(NULL));
		        fflush(logFile);
		        write(db_sock, db_log_msg, strlen(db_log_msg));
        	}
		else if(homeEmpty())
		{
			printf("Notice: User Gone Time:%u\n\n", (unsigned)time(NULL));
          		sprintf(sec_msg, "Type:switch;Action:on");
        	        // Send turn on message to security system
           		int x;
			if(num_gws == 2)
			{
        			for(x=0; x<gadget_index; x++)
        			{
        			        GADGET *gadget = gadget_list[x];
        			        
        		 	       if(strcmp (gadget-> gadgetType, SECURITYDEVICE) == 0)
        		 	       {
        					struct sockaddr_in addrDest;
        				
        					addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
        					addrDest.sin_family = AF_INET;
        					addrDest.sin_port = htons(gadget->port);
        					
        					if( sendto(gadget->id, sec_msg , strlen(sec_msg) , 0, 
        						(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
        					{
        						puts("Send failed");
        						break;
        					} 
        		      		        printf("Send: To: securitydevice Msg:%s Time:%u\n\n", sec_msg, (unsigned)time(NULL));

        		       	       }
        			}
			}
			else
			{
				for(x=0; x<global_index; x++)
        			{
        			        GADGET *gadget = global_list[x];
        			        
        		 	       if(strcmp (gadget-> gadgetType, SECURITYDEVICE) == 0)
        		 	       {
        					struct sockaddr_in addrDest;
        				
        					addrDest.sin_addr.s_addr = inet_addr(gadget->ip);
        					addrDest.sin_family = AF_INET;
        					addrDest.sin_port = htons(gadget->port);
        					
        					if( sendto(gadget->id, sec_msg , strlen(sec_msg) , 0, 
        						(struct sockaddr*)&addrDest, sizeof(addrDest)) < 0)
        					{
        						puts("Send failed");
        						break;
        					} 
        		      		        printf("Send: To: securitydevice Msg:%s Time:%u\n\n", sec_msg, (unsigned)time(NULL));

        		       	       }
        			}
			}	    
        	        char* gw_log_msg = "User Left Home\n";
		        char* db_log_msg = "Type:insert;Action:User Left Home!\n";
		        fprintf(logFile, "%s %u", gw_log_msg, (unsigned)time(NULL));
		        fflush(logFile);
		        write(db_sock, db_log_msg, strlen(db_log_msg));
		}
        }
        
        memset(out_msg, 0, sizeof(out_msg));
        
        write(client_skt_desc, out_msg, strlen(out_msg));

        memset(client_msg, 0, sizeof(client_msg));
    }

    if( read_size == 0 ) 
    {
        char msg[100];
        sprintf(msg, 
                "Client %d Unregistered...", 
                client_skt_desc);
        //puts(msg);
        fflush(stdout);
    } 
    
    else if ( read_size == -1 ) 
    {
        perror("Data Read Failed");
    }

    return 0;
}

int main( int argc, char *argv[] )
{
    FILE *fp = fopen(argv[1],"r");

    if ( NULL == fp ) 
    {
        perror("Gateway Configuration File Read Failure");
        return 1;
    }

    logFile = fopen(argv[2],"a");

    if ( NULL == logFile )
    {
        perror("Gateway Log File Read Failure");
        return 1;
    }

    char* line1 = NULL;
    ssize_t len1 = 0;
    char* line2 = NULL;
    ssize_t len2 = 0;

    getline(&line1, &len1, fp);
    getline(&line2, &len2, fp);

    char *token, *ip_cur, *ip_pri;
    unsigned short int port_cur, port_pri;

    //Parse Current Gateway info
    token = strtok(line1, ",");

    ip_cur = token;

    if( NULL != token )
    {
        token = strtok(NULL, ",");
    }

    port_cur = (unsigned short int) atoi(token);

    //Parse Primary Gateway info
    token = strtok(line2, ",");

    ip_pri = token;

    if( NULL != token )
    {
        token = strtok(NULL, ",");
    }

    port_pri = (unsigned short int) atoi(token);

    //This GW is the primary (needs to open socket to listen for secondary IP/Port)
    if(strstr(ip_cur, ip_pri) && port_cur == port_pri)
    {
	puts("I am Primary!");
	amPri = 1;
	gw_pri_ip = malloc(sizeof(char) * strlen(ip_pri));
	strcpy(gw_pri_ip, ip_pri);
	gw_pri_port = port_pri;

	int pri_skt_desc;

    	// Socket Addresses Data Type
   	struct sockaddr_in server_pri;
	socklen_t size = sizeof(pri_skt);
	sock_pri = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
	if (sock_pri == -1)
        {
        	printf("Could not create socket");
    	}

	puts("Socket created");
     
	int tru = 1;
	if(setsockopt(sock_pri, SOL_SOCKET, SO_REUSEADDR, &tru, sizeof(int)) == -1)
	{
		perror("Sock Option Error");
		exit(1);
	}

    	server_pri.sin_addr.s_addr = htonl(INADDR_ANY);
    	server_pri.sin_family = AF_INET;
    	server_pri.sin_port = htons( gw_pri_port );

	if( bind(sock_pri, (struct sockaddr *) &server_pri, sizeof(server_pri)) < 0 )
    	{
     	   perror("Bind Failed");
     	   return 1;
   	}

	char buf[MSG_SIZE];
	int recv_len;
	if(recv_len = recvfrom(sock_pri, buf, MSG_SIZE, 0, (struct sockaddr*) &pri_skt, &size) < 0)
	{
		perror("Error Receiving Data!");
		exit(1);
	}

	printf("BUFFER: %s\n", buf);

	char* token;
	char* token2;
	token = strtok(buf, ",");
	gw_sec_ip = malloc(sizeof(token));
	strcpy(gw_sec_ip, token);
	token2 = strtok(NULL, "");
	gw_sec_port = atoi(token2);

	close(sock_pri);

    }  //This GW is the Secondary meaning we have all the information
    else if(!strstr(ip_cur, ip_pri) || port_cur != port_pri)
    {
	puts("I am Secondary!");
	gw_sec_ip = malloc(sizeof(char) * strlen(ip_cur));
	strcpy(gw_sec_ip, ip_cur);
	gw_sec_port = port_cur;
	gw_pri_ip = malloc(sizeof(char) * strlen(ip_pri));
	strcpy(gw_pri_ip, ip_pri);
	gw_pri_port = port_pri;

    	struct sockaddr_in server_pri;
	int size = sizeof(server_pri);
	sock_sec = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP);
	if (sock_sec == -1)
        {
        	printf("Could not create socket");
    	}

	puts("Socket created");
     
    	server_pri.sin_addr.s_addr = inet_addr( gw_pri_ip );
    	server_pri.sin_family = AF_INET;
    	server_pri.sin_port = htons( gw_pri_port );

	char sec_info[MSG_SIZE];
	sprintf(sec_info, "%s,%u", gw_sec_ip, gw_sec_port);
	sendto(sock_sec, sec_info, strlen(sec_info), 0, (struct sockaddr*) &server_pri, size);
	printf("SENT: %s\n", sec_info);
	close(sock_sec);
    }

    fclose(fp);

    // Socket Descriptor
    int skt_desc;

    int client_skt_desc;

    size_t size;

    // Socket Addresses Data Type
    struct sockaddr_in skt, client_skt;

    // Socket Creation
    if( (skt_desc = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
        perror("Error creating socket");
        return 1;
    }
    
    puts("Socket Created...");

    // Set host machine address as Any Incoming Address
    skt.sin_addr.s_addr = inet_addr( ip_cur );

    // Set socket address format
    skt.sin_family = AF_INET;

    // Set port number
    skt.sin_port = htons( port_cur );

    // Bind
    if( bind(skt_desc, (struct sockaddr *) &skt, sizeof(skt)) < 0 )
    {
        perror("Bind Failed");
        return 1;
    }

    puts("Bind...");

    // 3 Max Connections to queue for socket
    if( listen(skt_desc, MAX_CONNECTIONS) < 0 )
    {
        perror("Error while listening");
        return 1;
    }
 
    puts("Listening...");

    size = sizeof( struct sockaddr_in );

    pthread_t thread;
    
    memset(&vectorclock, 0, sizeof(vectorclock));
    vectorclock = (struct VECTORCLOCK){0};
    // Initilize Vector Clock
    vectorclock.door = 0;
    vectorclock.motion = 0;
    vectorclock.keyChain = 0;
    vectorclock.gateway = 0;
    vectorclock.securitySystem = 0;

    //Start the heartbeat messages between GWs
    pthread_t hbeat1S_thread, hbeat2S_thread;
    pthread_t hbeat1R_thread, hbeat2R_thread;
    if(amPri)
    {
            if( pthread_create(&hbeat1R_thread, NULL, (void *) &heartbeat_recv, NULL) < 0 )
            {
                perror("Thread Creation Failed");
            }

	    sleep(1);

 	    if( pthread_create(&hbeat1S_thread, NULL, (void *) &heartbeat_send, NULL) < 0 )
            {
                perror("Thread Creation Failed");
            }
    } 
    else 
    {
            if( pthread_create(&hbeat2R_thread, NULL, (void *) &heartbeat_recv, NULL) < 0 )
            {
                perror("Thread Creation Failed");
            }

	    sleep(1);

	    if( pthread_create(&hbeat2S_thread, NULL, (void *) &heartbeat_send, NULL) < 0 )
            {
                perror("Thread Creation Failed");
            }
    }
    
    // Complete Connection with a Client
    while( client_skt_desc = accept(skt_desc, (struct sockaddr *) &client_skt, (socklen_t *) &size) )
    {
        if( client_skt_desc < 0 ) 
        {
            perror("Connection Failed");
            return 1;
        }

        puts("Connection Accepted...");

        if( pthread_create( &thread, NULL, connection, (void*) &client_skt_desc) < 0 )
        {
            perror("Thread Creation Failed");
            return 1;
        }
    }

    fclose(logFile);
    return 0;         
}
