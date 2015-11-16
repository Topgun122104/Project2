#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "gadgets.h"

// List of Devices and Sensors
GADGET *gadget_list[MAX_CONNECTIONS];
struct VECTORCLOCK vectorclock;
int gadget_index = 0;
int db_sock = -1;
FILE *logFile;

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
	
	if(NULL != temp)
	{
		t = strtok(NULL, ",");
		vectorclock.keyChain = max(vectorclock.keyChain, atoi(t));
	}
	
	t = strtok(temp, ",");
	vectorclock.gateway++;
	
	if(NULL != temp)
	{
		t = strtok(temp, ",");
		vectorclock.securitySystem = max(vectorclock.securitySystem, atoi(t));
	}
	
    char vc[MSG_SIZE];
    sprintf(vc, "Gateway VectorClock:%d,%d,%d,%d,%d,\n",
			vectorclock.door, vectorclock.motion,
			vectorclock.keyChain, vectorclock.gateway,
			vectorclock.securitySystem);
}

int max(int x, int y) {
	if(x >= y) 
		return x;
	else return y;
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
	char tempVal[6];
	if(strstr(type, MOTION) || strstr(type, KEYCHAIN))
	{
		if(val)
		{
			strcpy(tempVal, "True");
		}
		else
		{
			strcpy(tempVal, "False");
		}
	}
	else if(strstr(type, DOOR))
	{
		if(val)
		{
			strcpy(tempVal, "Open");
		}
		else
		{
			strcpy(tempVal, "Close");
		}
	}

	char *str = (char*)malloc(sizeof(char) * 100);
	if(strstr(type, SECURITYDEVICE))
	{
		sprintf(str, "Type:insert;Action:%d,%s,%s,%u,%s,%d\n",
                    id, type, sta, (unsigned)time(NULL), ip, port);
	}
	else
	{
		sprintf(str, "Type:insert;Action:%d,%s,%s,%u,%s,%d\n",
                    id, type, tempVal, (unsigned)time(NULL), ip, port);
	}

	return str;
}

// Send multicast to all devices, except database, with list of messages
void sendDeviceListMulticast() {
	
	char deviceList[MSG_SIZE];
	int x;
	strcpy(deviceList, "DeviceList");
	
	// Creating the device list to send to all devices
    for(x=0; x<gadget_index; x++)
    {
        GADGET *gadget = gadget_list[x];
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
    for(x=0; x<gadget_index; x++)
    {
        GADGET *gadget = gadget_list[x];
        
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
}

// Print the latest information about Devices and Sensors
void printGadgets()
{
	
    puts("------------------------------------------------");

    int x;
    for(x=0; x<gadget_index; x++)
    {
        GADGET *gadget = gadget_list[x];

        char msg[MSG_SIZE];
	char* currStr = toString(gadget->currValue, gadget->gadgetType);

        if(isOn(gadget->state))
	{
		//If Security Device, we want to display current state instead of current value
		if(strstr(gadget->gadgetType, SECURITYDEVICE) || strstr(gadget->gadgetType, DATABASE))
		{
			puts("Inside secDev state");
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

    puts("------------------------------------------------");
}

// Get the Type and Action from a received command
void getCommands(char string[], char **type, char **action)
{
	printf("command: %s\n", string);
    char *tok, *tok2;

    tok = strtok(string, ";");

    if( NULL != tok )
        tok2 = strtok(NULL, ";");

    tok = strtok(tok, ":");

    if( NULL != tok )
        *type = strtok(NULL, ":");

    tok2 = strtok(tok2, ":");

    if( NULL != tok2 )
        *action = strtok(NULL, ":");
    
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

// Gadget's Thread
void *connection(void *skt_desc)
{
    int client_skt_desc = * (int *) skt_desc;

    GADGET *gadget = malloc(sizeof(GADGET));
    
    gadget->id = client_skt_desc;

    int read_size;
    char *command, *action;

    char client_msg[MSG_SIZE], msg[MSG_SIZE], out_msg[MSG_SIZE], cpy_msg[MSG_SIZE];
    char* log_msg;
    char vc[MSG_SIZE];

    // Initilize Vector Clock
    vectorclock.door = 0;
    vectorclock.motion = 0;
    vectorclock.keyChain = 0;
    vectorclock.gateway = 0;
    vectorclock.securitySystem = 0;
    
    // Receive Data from Client
    while( (read_size = recv(client_skt_desc, client_msg, MSG_SIZE, 0)) > 0 )
    {
        memset(msg, 0, sizeof(msg));
        memset(cpy_msg, 0, sizeof(cpy_msg));
        strncpy(cpy_msg, client_msg, sizeof(client_msg));
        
        printf("\nFROM CLIENT:\n%s\n\n",client_msg);

        getCommands(client_msg, &command, &action);

        // Register Case
        if( strncmp(command, CMD_REGISTER, strlen(CMD_REGISTER)) == 0 )
        {
            char *t_gadgetType, *t_ip;
            
            getInfo(action, &t_gadgetType, &t_ip, &gadget->port, &gadget->area);
            
            gadget->gadgetType = (char *)malloc(sizeof(char) * strlen(t_gadgetType));
            memcpy(gadget->gadgetType, t_gadgetType, strlen(t_gadgetType));

            gadget->ip = (char *)malloc(sizeof(char) * strlen(t_ip));
            memcpy(gadget->ip, t_ip, strlen(t_ip));
        
            gadget->state = (char *)malloc(sizeof(char) * 3);            
            strcpy(gadget->state, ON);

            gadget_list[gadget_index++] = gadget;

            // Creating list of the ports and ips to send to all devices            
            if (gadget_index == 5) {
                sendDeviceListMulticast();
            }
            
            printGadgets();
            
			if(strstr(gadget->gadgetType, DATABASE))
			{
				puts("Inside if...");
				db_sock = client_skt_desc;
				printf("db_sock: %d\n", db_sock);
			}
        }

        // Current Temperature Value Case
        else if( strncmp(command, CMD_VALUE, strlen(CMD_VALUE)) == 0 ) 
        {
    		printf("Updating clock2... %s\n", cpy_msg);
    		if(strncmp(cpy_msg, "VectorClock", 11) == 0)
    			updateVectorClock(cpy_msg);
    		
            if( atoi(action) != gadget->currValue )
            {
                puts("CHANGING"); 
                gadget->currValue = atoi(action);
                printGadgets();
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
    		printf("Updating clock... %s\n", cpy_msg);
    		if(strncmp(cpy_msg, "VectorClock", 11) == 0)
    			updateVectorClock(cpy_msg);
    		
            memset(gadget->state, 0, 3);
            strcpy(gadget->state, action);
            printGadgets();
			log_msg = generateDBMsg(client_skt_desc, gadget->gadgetType, gadget->state, gadget->currValue, gadget->ip, 							gadget->port);
			fprintf(logFile, "%s", log_msg);
			fflush(logFile);
			write(db_sock, log_msg, strlen(log_msg));
        }

        memset(out_msg, 0, sizeof(out_msg));
        
            if( !isOn(gadget->state) )
            {
                sprintf(out_msg,
                        "Type:%s;Action:%s",
                        CMD_SWITCH, ON);
                gadget->state = "on";
            }
            write(client_skt_desc, out_msg, strlen(out_msg));

        memset(client_msg, 0, sizeof(client_msg));
	
    }

    if( read_size == 0 ) 
    {
        char msg[100];
        sprintf(msg, 
                "Client %d Unregistered...", 
                client_skt_desc);
        puts(msg);
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

    fseek(fp, 0, SEEK_END);
    long pos = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *gateway = malloc(pos * sizeof(char));
    fread(gateway, pos, 1, fp);

    char *token, *ip;
    unsigned short int port;

    token = strtok(gateway, ",");

    ip = token;

    if( NULL != token )
    {
        token = strtok(NULL, ",");
    }

    port = (unsigned short int) atoi(token);

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
    skt.sin_addr.s_addr = inet_addr( ip );

    // Set socket address format
    skt.sin_family = AF_INET;

    // Set port number
    skt.sin_port = htons( port );

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
