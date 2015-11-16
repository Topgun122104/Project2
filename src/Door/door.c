#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

#include "gadgets.h"

GADGET *gadget_list[MAX_CONNECTIONS];
int gadget_index = 0;
int *input;
int *inter;
int inter_size;
int interIndex = 0;
int array_size; 

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
		strcpy(str, "Open");
	}
	else
	{
		strcpy(str, "Close");
	}

	return str;
}

int isOn(char *state)
{
    return ( strncmp(state, ON, strlen(ON)) == 0 ) ? 1 : 0;
}

void *timer()
{
    FILE *fp = fopen("../SampleConfigurationFiles/SampleDoorInput.txt","r");

    if ( NULL == fp )
    {
        perror("Door Input File Read Failure");
    }

    // Get the last line
    char *last;
    size_t last_len = 0;
    
    while(!feof(fp))
    {
        getline(&last, &last_len, fp);
  	inter_size++;
    }

    //Terrible fix to wierd bug...Don't touch!
    inter_size--;
    inter_size--;
    //////////////////////////////////////////

    printf("Inter_size: %d\n", inter_size);

    fseek(fp, 0, SEEK_SET);

    char *last_tok;

    last_tok = strtok(last, ",");
    
    if( NULL != last_tok )
        array_size = atoi(last_tok);   

    char* temp;
    char* temper = (char *)malloc(sizeof(char) * 6);
    size_t line_len = 0;
    int startTime, endTime, val;

    input = malloc((array_size + 1) * sizeof(int));
    inter = malloc((inter_size - 1) * sizeof(int));
    
    int flag = 1;
    while( getline(&temp, &line_len, fp) > 0 )
    {
    	if(flag)
	{
		char *token;

        	token = strtok(temp, ",");

        	startTime = atoi(token);

        	if( NULL != token )
            		strcpy(temper, strtok(NULL, ""));

		printf("Start: %d  Temper: %s\n", startTime, temper);

		flag = 0;
	}
	else
	{
		char *token;

        token = strtok(temp, ",");
        
        endTime = atoi(token);

		printf("Start: %d End: %d   ", startTime, endTime);

		//Convert Open or Close to 1 or 0 respectively
        if(strstr(temper, OPEN))
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

		inter[interIndex++] = endTime - startTime;

		startTime = endTime;
		if( NULL != token )
          	  strcpy(temper, strtok(NULL, ""));
	}
    }
    
    fclose(fp);

    int x;
    for(x=0;x<array_size;x++)
    {
        printf("%d",input[x]);
    }
	puts("\n");

    int y;
    for(y=0;y<inter_size;y++)
    {
        printf("%d ",inter[y]);
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

    //Populate the global arrays
    timer();

    // Sensor State (On by default)
    char *state = ON;

    char *type, *action;

    int currValue = 0;
    int oldValue = 0;
    int intervalIndex = 0;

    // Wait interval (in seconds)
    int actualTime = 0;

    // Register Message
    char msg[MSG_SIZE];
    char log_msg[MSG_SIZE];

    sprintf(msg,
            "Type:register;Action:%s-%s-%d-%d",
            s_type, s_ip, s_port, s_area);

    // Initialize the vector clock, all counters are 0
    VECTORCLOCK *vectorclock = malloc(sizeof(VECTORCLOCK));
    vectorclock->door = 0;
    vectorclock->motion = 0;
    vectorclock->keyChain = 0;
    vectorclock->gateway = 0;
    vectorclock->securitySystem = 0;
    
    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    while(1)
    {
        printf("\nSend to Gateway: %s\n",msg);
        // Send Gateway Current Monitored Value
        if( send(sock , msg , strlen(msg) , 0) < 0)
        {
            puts("Send failed");
            break;
        }
        
        // Receive multicast messages from other devices
        if( recv(sock , server_reply , MSG_SIZE , 0) > 0)
        {
            puts("Gateway reply:");
            puts(server_reply);
            
        	// The device list multicast message
        	if( strncmp( server_reply, "DeviceList", 10) == 0)
        		saveDevices(server_reply, s_ip, s_port);
        }

		printf("Current Interval: %d\n", inter[intervalIndex]);
		
		// Wait for time interval (5 seconds default)
		int currInterval = inter[intervalIndex++];
			sleep(currInterval);
		
		//Keep track of index in input array
		actualTime += currInterval;
	
		//Make sure input doesn't go out of bounds
		if(actualTime > array_size)
			actualTime -=array_size;
	
		//Make sure interval doesn't go out of bounds
			if(intervalIndex >= inter_size)
				intervalIndex = 0;
	
		//Get value at actual point in input array
			currValue = input[actualTime];
	
			memset(msg, 0, sizeof(msg));

		printf("Current Value: %d\nOld Value: %d\n", currValue, oldValue);
	
		//Only send door state when state changes
			if(currValue != oldValue)
			{            
			sprintf(msg,
						"Type:currValue;Action:%d",
						currValue);
			}
	
		char* v = toString(currValue);
		sprintf(log_msg, "%d,%s,%s,%u,%s,%d\n",
						sock, s_type, v, (unsigned)time(NULL), s_ip, s_port);
		fprintf(log, "%s", log_msg);
		fflush(log);
		//Update the values
		oldValue = currValue;
    }
     
    fclose(log);
    close(sock);
    return 0;
}
