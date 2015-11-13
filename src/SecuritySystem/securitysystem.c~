#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "gadgets.h"

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
    FILE *log = fopen("../src/SecuritySystem/system.log","a");

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
    
    sprintf(msg, 
            "Type:register;Action:%s-%s-%d-%d",
            d_type, d_ip, d_port, d_area);
    
    // Current Device State (OFF by default)
    char *state = (char *)malloc(sizeof(char) * 3);
    strcpy(state,OFF);

    while(1)
    {
        // Send Message to Gateway
        if( send(sock , msg , strlen(msg) , 0) < 0)
        {
            puts("Send failed");
            break;
        }
        
        printf("\nSent: %s\n",msg);

        // Receive Command from Gateway
        if( recv(sock , server_reply , MSG_SIZE , 0) < 0)
        {
            puts("Gateway reply failed");
            break;
        }
         
        puts("Gateway reply:");
        puts(server_reply);

        getCommands(server_reply,&type,&action);

	if ( strncmp( type, CMD_SWITCH, strlen(CMD_SWITCH) ) == 0 )
        {
            puts("Device Switching State");
            strcpy(state, action);
            puts("NEW STATE:");
            puts(state);

	    sprintf(log_msg, "%d,%s,%s,%u,%s,%d\n",
                    sock, d_type, state, (unsigned)time(NULL), d_ip, d_port);
	    fprintf(log, "%s", log_msg);
	    fflush(log);
        }	

        memset(msg, 0, sizeof(msg));
        memset(server_reply, 0, sizeof(server_reply));
        sprintf(msg,
                "Type:%s;Action:%s",
                CMD_STATE, state);
    }
    
    fclose(log); 
    close(sock);
    return 0;
}
