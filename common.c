/*
 * @akannan4_proj1.c
 * @author  Abhishek Kannan 
 * @email   akannan4@buffalo.edu
 *
 * Common functions for server and client
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include <ifaddrs.h>

#include <unistd.h> 

#include <arpa/inet.h>
#include <netinet/in.h>
#include <bits/local_lim.h>
#include <netdb.h>


// Node
typedef struct
{
    char* hostname;
    char* IPAddress;
    int port; //actual port
    int socketFd;
    int listenPort; // listen port
    struct node *next;
}node;

// Structure for broadcast
typedef struct 
{
    // Suggested on stack overflow 
    // sending this structure through sockets! pointers doesn't work
    char hostname[256]; 
    char IPAddress[256];
    int port; 
}peer;

// Mapping commands to integers for ease
char* commands[11] = {"help","creator","display","register","connect","list","terminate","quit","get","put","sync"};

/*
 * Function:  push 
 * --------------------
 * Adds an entry to the IP List 
 *
 *  address:        IP Address to insert
 *  fd:             Socket file descriptor
 *  listenPort:     Listening port number
 *  list:           IP List
 *
*/    

void push(struct sockaddr_in address,int fd,int listenPort,node** list){

    //create new node to save client information
    node *newNode = (node*)malloc(sizeof(node));

    //check if memory for new node has been allocated
    if(newNode == NULL){
        fprintf(stderr, "Unable to allocate memory for new node\n");
        exit(-1);
    }


    //fill client information in the node
    newNode->hostname = (char*) malloc(sizeof(char)*HOST_NAME_MAX) ;
    getnameinfo((struct sockaddr*)&address, sizeof(address),newNode->hostname,HOST_NAME_MAX, NULL, 0, 0);

    newNode->IPAddress = (char*) malloc(sizeof(char)*INET6_ADDRSTRLEN);
    inet_ntop(AF_INET, &(address.sin_addr), newNode->IPAddress, INET6_ADDRSTRLEN);
    
    newNode->port = ntohs(ntohs(address.sin_port));
    newNode->listenPort = listenPort;
    newNode->socketFd = fd;
    newNode->next = NULL;
    node *current = *list;

    if(*list ==NULL){
        (*list)=newNode;


    }
    else{

      while (current->next) {
        current = (node*)current->next;
    }    
     current->next = (node*)newNode;
     }
}


/*
 * Function:  displayList 
 * --------------------
 * Displays IP List
 *
 *
 *  list:       IP List
 *               
*/    

void displayList(node* list)
{
    int connID=1;
    printf("Conn ID\t\t Hostname\t\tIP Address\t\tPort No.\n");
    while(list!=NULL)
    {
        printf("%d\t\t%s\t\t%s\t%d\n",connID++,list->hostname,list->IPAddress,list->listenPort);
        list = (node*) list->next;
    }


}

// Source: stack overflow
// Logic for delete from stack overflow
/*
 * Function:  delete 
 * --------------------
 * Deletes an entry from the IP List based on IP Address and Port number
 *
 *  ip:         IP Address to delete
 *  port:       Port number
 *  list:       IP List
 *
 *  returns:    1 if delete was successful
 *              0 if delete was unsucessful
 *               
*/    
int delete(char* ip,int port, node** list){


    node* curr = *list;
    node* prev = *list;

    
    while(curr)
    {
        if(strcmp(ip,curr->IPAddress)==0 && (curr->port == port))
        {


            if(curr == *list && curr->next) {
                *list =(node*) curr->next; 
            }
            else if(curr->next) {
                prev->next =(node*) (curr)->next; 
            }
            else if(!curr->next) {
    
                prev->next = NULL;
                
            }

            return 1; // delete succeeded
        }

        // iterate
        prev = curr;
        curr =(node*) curr->next;
    }
    return 0; //delete failed
}




/*
* Function:  help 
* --------------------
* Displays help
*
*/    

void help() {
    printf("All commands are case sensitive, Following are the list of available commands\n");
    printf("\tHELP    - Displays information about the available user command options.\n");
    printf("\tCREATOR - Displays the author's full name, UBIT name and UB email address.\n");
    printf("\tDISPLAY - Display the IP address of this process, and the port on which this process is listening for incoming connections.\n");
    
    printf("\tCONNECT <destination> <port no> - Connects to <destination> at port <port no>\n");
    printf("\tREGISTER <server IP> <port no> - Registers with server at given IP Address and port\n");
    
    printf("\tTERMINATE <connection id> - Terminates the connection with <connection id> begotten using list command\n");
    printf("\tEXIT - Shutdown\n");
}
    
/*
* Function:  creator 
* --------------------
* Displays creator
*
*/

void creator(){
    printf("Full Name : Abhishek Kannan\n");
    printf("UBIT Name: akannan4\n");
    printf("UB email  : akannan4@buffalo.edu\n");
}

/*
* Function:  display 
* --------------------
* Displays machine's IP address
*
*   port:       Port number on which this process is listening
*   display:    Flag to indicate if IP Address should be printed to standard output
*
*/

// Source: http://jhshi.me/2013/11/02/how-to-get-hosts-ip-address/
int display(char* port,int display){


    char* target_name = "8.8.8.8";
    char* target_port = "53";

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* info;
    int ret = 0;
    if ((ret = getaddrinfo(target_name, target_port, &hints, &info)) != 0) {
        printf("[ERROR]: getaddrinfo error: %s\n", gai_strerror(ret));
        return -1;
    }

    if (info->ai_family == AF_INET6) {
        printf("[ERROR]: do not support IPv6 yet.\n");
        return -1;
    }

    int sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sock <= 0) {
        perror("socket");
        return -1;
    }

    if (connect(sock, info->ai_addr, info->ai_addrlen) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    struct sockaddr_in local_addr;
    socklen_t addr_len = sizeof(local_addr);
    if (getsockname(sock, (struct sockaddr*)&local_addr, &addr_len) < 0) {
        perror("getsockname");
        close(sock);
        return -1;
    }

    char * myip=(char*) malloc(sizeof(char)*INET6_ADDRSTRLEN);
    if (inet_ntop(local_addr.sin_family, &(local_addr.sin_addr), myip, INET6_ADDRSTRLEN) == NULL) {
        perror("inet_ntop");
        return -1;
    }
    if(display ==1)
        printf("IP Address %s Port %s \n",myip,port);
    myIP=myip;
}

/*
* Function:  quit 
* --------------------
* Terminates all connections!
*
*   list: List containing all client's information
*
*/
void quit(node** list){

    node* curr = *list;
    int fd;

    while(curr){
        fd=curr->socketFd;
        shutdown(fd,SHUT_RDWR);
        if(close(fd)== 0){ //Termiate successful
            delete(curr->IPAddress,curr->port,list);
            printf("Terminated connection to %s \n",curr->hostname);
        }
        
        
        curr=(node*) curr->next;


    }

    printf("Terminated all connections. I'm going down! \n");

 }

 /*
* Function:  getMaxFD 
* --------------------
* Get maximum file descriptor
*
*   list: List containing information about all connected clients
* 
*   returns: maximum file descriptor
*/

// stack overflow
int getMaxFD(node* list) {

    node* curr = list;
    int maxFD = 0;
    while(curr) {
        if(maxFD<curr->socketFd) 
            maxFD = curr->socketFd;
        curr = curr->next;
    }
    return maxFD;
}




/*
 * Function:  parse 
 * --------------------
 * Parses the command entered by the user and checks if it's a valid command.
 * Tokenizes the command and saves it in the global variable "parsedCommand"
 *
 *  cmd:        Command entered by user
 *
 *  returns:    cmdNo if command is valid
 *              -1 if command is invalid
 *               
*/    

int parse(char* cmd){
    // Strip newline from STDIN. Is this really required ?

    int numberOfArgs=0;
    int x=strcspn(cmd, "\n");
    if(x>0)
        cmd[x] = '\0';
    

    int argsCount=0;
    char** args = (char**) malloc(3*sizeof(char*));
    int cmdNo=0;

    args[argsCount] = strtok(cmd," ");
    int length = strlen( args[argsCount] ); 
    char* cmdLower = ( char* )malloc( length +1 ); 
    int i;
    for(i = 0; i < length; i++ ) {
        cmdLower[i] = tolower( args[argsCount][i] );
    }
    //printf("Command in lower case is: %s\n",cmdLower);
    cmdLower[length]='\0';

    // check if command is valid
    for(cmdNo=0;cmdNo<11;cmdNo++) {

        if(strcmp(cmdLower,commands[cmdNo]) ==0) {

            break;
        }
    }
    //printf("%s\n",args[argsCount]);
    //printf("%s\n",cmdLower);
    if(cmdNo > 10) // Invalid command
    {
        printf("Invalid command \n");
        return -1;
    }
    
    while( args[argsCount] != NULL ) {
        numberOfArgs++;
    
      args[++argsCount] = strtok(NULL, " ");
   }
   parsedCommand = args;

   // check if usage is correct!

   if (cmdNo==3 || cmdNo== 4 || cmdNo==8 || cmdNo ==9){
        if (numberOfArgs==3)
        return cmdNo;
    else {
        printf("Invalid command - Wrong Arguments \n");

        return -1;
    }
   }

    if (cmdNo==6){
        if (numberOfArgs==2)
        return cmdNo;
    else {
        printf("Invalid command - Wrong Arguments\n");

        return -1;
    }
   }


   return cmdNo;

}





