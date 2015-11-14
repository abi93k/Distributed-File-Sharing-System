/*
 * @akannan4_proj1.c
 * @author  Abhishek Kannan 
 * @email	akannan4@buffalo.edu
 *
 * Main program
*/

#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>

#include "common.c"
#include "client.c"
#include "server.c"


char** parsedCommand;
char* myIP;

/*
 * Function:  main 
 * --------------------
 * Main function
 *
 *	returns 0 if failed to created server/client
 *	returns 1 if application has been closed
 *	
*/    
int main(int argc, char** argv)
{

	if(argc != 3)
	{
		printf("Usage ./a.out (s/c) port\n");
		
		return 0;
	}

	char* mode = argv[1];
	char* port = argv[2];
	display(port,0);



	switch(*mode)
	{
		case 's':
			server(port);
			break;
		case 'c':
			client(port);
			break;
		default:
			printf("Mode can be either 's' (server) or 'c' (client)\n");
			return 0;
	}
	return 1;
}