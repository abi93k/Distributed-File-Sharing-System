/*
 * @akannan4_proj1.c
 * @author  Abhishek Kannan 
 * @email   akannan4@buffalo.edu
 *
 * Server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/wait.h>
#include <signal.h>


/*
* Function:  syncRelay 
* --------------------
* Relays the SYNC request to a client
*
*  
*   curr: Pointer to an entry in the Server IP List
*
*/
void syncRelay(node* curr){

    char * s="SYNC";

	send(curr->socketFd,s,sizeof(s),0);
}

/*
* Function:  server 
* --------------------
* Function to start server
*
*   port:       Port number to which server should be bound
*   
*   returns:    1 if server was shutdown
*				0 if some error was encountered
*          
*/


int server(char* port) {

	int completed=0;


	node *IP_list = NULL;
	int noOfConnections=0;
	int cmdNo;

	fd_set read_fds, write_fds,read_fds_copy,write_fds_copy;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	
	//int fdMax = STDIN_FILENO;
	int fdMax = 0; // 0 is file descriptor for standard input 
	int serverSocket, nSocket,selected;  
	struct sockaddr_in serverAddress,clientAddress;
	struct sockaddr_storage getPeerInfo;
	socklen_t clientAddrLen = sizeof(clientAddress);

	char msg[5000]; 
	int numBytes;

	serverSocket= socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);  

	printf("Server created \n");
	
	if(serverSocket == -1) {
		printf("Server: Socket creation failed\n");
	}

	fdMax = serverSocket;
	/* Beej: Allows other sockets to bind() to this port, unless there is an active listening socket bound to the port already. 
	This enables you to get around those "Address already in use" error messages when you try to restart your server after a crash. */
	int socketReuse = 1; 
	setsockopt(serverSocket,SOL_SOCKET,SO_REUSEADDR,&socketReuse,sizeof(int));


	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(atoi(port)); //network Byte order

	// bind
	if(bind(serverSocket,(struct sockaddr*)&serverAddress,sizeof(serverAddress))== -1) {
		printf("Server: Binding failed\n");
		close(serverSocket); 
	}
	printf("Server has been bound to port %d \n",atoi(port));

	//printf("Server sockets is %d \n",serverSocket);

	listen(serverSocket,10);

	FD_SET(STDIN_FILENO,&read_fds);
	FD_SET(serverSocket,&read_fds);
	
	while(1) {
		// Create copies of fd_sets because select will modify them
		read_fds_copy = read_fds;
		write_fds_copy = write_fds;
		if (select(fdMax+1, &read_fds_copy,NULL, NULL, NULL) == -1) {
			perror("Server : Select error");
			continue;
		}
		for(selected = 0;selected<=fdMax;selected++) {
			if(FD_ISSET(selected,&read_fds_copy)) {
				/* Need to handle  3 scenarios
				1. Data available from Server socket (New Client arrives)
				2. Data available from Standard Input (Commands)
				3. Data available from clients (SYNC? )
				*/
				if(selected==serverSocket){ // Scenario 1
					printf("Server : A client is trying to connect.\n");
					
					memset(&clientAddress, 0, sizeof(clientAddrLen));
					if ((nSocket = accept(serverSocket,(struct sockaddr*)&clientAddress,&clientAddrLen))==-1) {
						printf("Server: Accept failed\n");
					}
					
					if (noOfConnections >= 4) {
						close(nSocket);
						printf("Server : Connection limit exceeded!\n");
						continue;
					}

					memset(msg,0,5000);
					// receive clients listening port number
					numBytes = recv(nSocket,msg,5000,0);
					printf("New connection from %s : %d \n",inet_ntoa(clientAddress.sin_addr),atoi(msg));
					printf("Client has been registered .Yay!\n");


					setsockopt(nSocket,SOL_SOCKET,SO_REUSEADDR,&socketReuse,sizeof(int));
					push(clientAddress,nSocket,atoi(msg),&IP_list);
										noOfConnections++;

					sendAll(IP_list,noOfConnections);
					FD_SET(nSocket,&read_fds);
					
					if(nSocket > fdMax) 
						fdMax = nSocket;
				} // End Scenario 1
				if(selected==0){ // Scenario 2
					memset(msg,0,sizeof(msg)); // clear msg array
					if ((numBytes = read(selected, msg, sizeof(msg))) <= 0)
						perror("Server read error");

					cmdNo=parse(msg);
					int fd;

					switch(cmdNo){
						case 0: // HELP
						help();
						break;
						case 1:
						creator(); //CREATOR
						break;
						case 2:
						display(port,1); //DISPLAY 
						break;
						case 3: //REGISTER
						printf("REGISTER command not available for server\n");
						break;
						case 4: //CONNECT 
						printf("CONNECT command not available for server\n");
						break;
						case 5: //LIST
						displayList(IP_list); 
						break;
						case 6: //TERMINATE 
						fd=terminate(&IP_list);
                            if(fd!=-1){
                                FD_CLR(fd,&read_fds);
                                FD_CLR(fd,&write_fds);

                                fdMax = getMaxFD(IP_list);
                                noOfConnections--;
                                if(noOfConnections==0){
                                    IP_list=NULL;
                                }

                            }
                            else{
                                printf("Invalid connection ID \n");

                            }
						break;
						break;
						case 7: //QUIT
						quit(&IP_list);
						return 1;
						break;
						case 8: //GET
						printf("GET command not available for server\n");
						break;
						case 9: //PUT
						printf("PUT command not available for server\n");
						break;
						case 10: //SYNC
						printf("SYNC command not available for server\n");
						break;
					}
				} // End Scenario 2

                else { // Scenario 3

					if ((numBytes = recv(selected, msg, 5000, 0)) <= 0) {
						if (numBytes == 0)
						{

							if(getpeername(selected,(struct sockaddr*)&getPeerInfo,&clientAddrLen)<0)
							{
								perror("Server : Error\n");
								return 0;
							}

							struct sockaddr_in *s = (struct sockaddr_in *)&getPeerInfo;

							char* IPAdd = (char*)malloc(sizeof(char)*INET6_ADDRSTRLEN);
							int targetPort = ntohs(ntohs(s->sin_port));
							inet_ntop(AF_INET, &(s->sin_addr), IPAdd, INET6_ADDRSTRLEN);

							printf("Server: Connection closed by IP %s\n",IPAdd);

							
							delete(IPAdd,targetPort,&IP_list);
							FD_CLR(selected,&read_fds);
							close(selected);
							noOfConnections--;

							if(noOfConnections == 0){
								printf("All connections are closed\n");
								IP_list=NULL;}
							else
							{
								// Broad cast current peer list
								fdMax = getMaxFD(IP_list);
								sendAll(IP_list,noOfConnections);
								
							}
						}
						else
						{
							//perror("Server");
						}
					}
					else// we got some data from a client
					{


						char command1[4];
						char command2[6];
                        memcpy(command1,msg,4); // Extract first 3 characters from incoming data
                        memcpy(command2,msg,6); // Extract first 3 characters from incoming data


                        if(!(strcmp(command1,"SYNC")) || (!(strcmp(command1,"sync")))) { // Check if it is GET command


                        	printf("Triggering SYNC on all connected peers!\n");


                        	node *curr=IP_list;
                        	
							syncRelay(curr);
							memset(msg,0,5000); // reset read buffer
							curr=curr->next;
							completed++;
					



                        }


                        else if(!(strcmp(command2,"E_SYNC")) || (!(strcmp(command2,"e_sync")))) { // Check if it is GET command

                        	printf("ESYNC received\n");
                        	int i;

                        	node *curr=IP_list;


                        	if(completed!=noOfConnections){
                        		for(i=0;i<completed;i++){
                        			curr=curr->next;
                        		}
                        		syncRelay(curr);
                        		completed++;
                        	}
                        	else if(completed==noOfConnections){
                        		completed=0;
                        	}


							memset(msg,0,5000); // reset read buffer



                        }

					}
                }
			}
		}
	}
}

/*
* Function:  sendAll 
* --------------------
* Function to broadcast server IP List to all registered clients
*
*   list:       List containing all registered clients
*	size:		size of list
*          
*/

void sendAll(node* list,int size)
{
	node* curr=list;
	peer peers[size];
	int i=0;
	//copy linked list to normal structure so that they can be sent over sockets
	while(curr){ 
		

		strcpy (peers[i].hostname, curr->hostname);
		strcpy (peers[i].IPAddress, curr->IPAddress);
		peers[i].port = curr->listenPort;
		curr = curr->next;
		i++;

	}
	curr=list;



	while(curr) 
	{
		//printf("Sending Peer list to %d : %s\n",curr->socketFd,curr->IPAddress);
		send(curr->socketFd,peers,sizeof(peers),0);
		curr = curr->next;
	}
}

