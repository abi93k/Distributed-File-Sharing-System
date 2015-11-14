
/*
 * @akannan4_proj1.c
 * @author  Abhishek Kannan 
 * @email   akannan4@buffalo.edu
 *
 * Client
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include <time.h> 

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#define RECV_BUFSIZE 1000
#define SEND_BUFSIZE 1000


struct timeval startTime, endTime;
typedef struct fileData{
     char header[256];
    char data[SEND_BUFSIZE];
    int size;

}fileData;

/*
* Function:  printTime 
* --------------------
* Returns time in HH:MM:SS format
*
*  
*   return: time
*
*/

char* printTime(){

    time_t current_time;
    struct tm * time_info;
    char timeString[9];  

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(timeString, sizeof(timeString), "%H:%M:%S", time_info);
    return timeString;
}

/*
* Function:  getHostFromSock 
* --------------------
* Returns the hostname of a Socket descriptor
*
*  
*   fd:         Socket file descriptor
*   list:       IP List
*
*   returns:    Hostname   
*/
char* getHostFromSock(int fd,node* list){


    while(list!=NULL){
        if(list->socketFd==fd){ 
            return list->hostname;
        }
        list=(node*) list->next;

    }


}

 /*
* Function:  isValid 
* --------------------
* Checks if an IP Address(peer) is registered to the server
*
*  
*   IP:     IP Address
*   list:   IP List
*   size:   Size of list
*
*/
int isValid(char* IP, peer* list,int size) {
        
        int i;
        for(i=0;i<size;i++)
        {
            if(strcmp(IP,list[i].IPAddress)==0){
                return 1;
            }
        }

        return 0;
}

/*
 * Function:  syncFiles 
 * --------------------
 * Forwards SYNC request to server
 * 
 *   list: IP list
 *
 */

void syncFiles(node* list){

    node *curr=list;


    char * s="SYNC";

    send(curr->socketFd,s,sizeof(s),0);

    printf("Initiated SYNC\n");


}

/*
* Function:  endSyncFiles 
* --------------------
* Notifies the server that SYNC has been completed
*
*  
*   list: IP list
*
*/
void endSyncFiles(node* list){

    node *curr=list;


    char * s="E_SYNC";

    send(curr->socketFd,s,sizeof(s),0);

    printf("Completed SYNC\n");


}

/*
* Function:  get 
* --------------------
* Forwards the GET Request to the appropriate client using connection ID
*
*   list: IP List
*
*   returns:    1 if forward was sucessful
*               0 if forward failed/file requested from server               
*/

int get(node* list) {



    int connID=atoi(parsedCommand[1]);
    if(connID==1){
        printf("Files cannot be downloaded from server!");
        return 0;
    }

    char* fileName=parsedCommand[2];
    char request[100];
    memset(request,0,100);


    int i;
    for(i=1;i<connID;i++){
        list=(node*) list->next;
    }
        // parsed command is pointer, so copy to char array before sending it.
    strcpy(request,parsedCommand[0]);
    strcat(request, " ");
    strcat(request,parsedCommand[1]);
    strcat(request, " ");
    strcat(request,parsedCommand[2]);


    if(send(list->socketFd,request,100,0)>0) {
        return 1;
    }
    else {
        return 0;
    }
}

/*
* Function:  put 
* --------------------
* Sends the file 'File name' to 'connection ID' 
*
*
*   list:       IP List
*
*   returns:    1 if send was sucessful
*               0 if send failed               
*/



int put(node* list)
{




    int connID=atoi(parsedCommand[1]);
    char* fileName=parsedCommand[2];
    FILE *uploadfp=NULL;
    struct stat file_stat;
    char file_name_size[256];
    char send_buffer[SEND_BUFSIZE];



    int i;
    for(i=1;i<connID;i++){
        list=(node*) list->next;
    }




    uploadfp=fopen(fileName, "r");
    if(uploadfp==NULL) {
        printf("the requested file does not exist: %s\n",fileName);
        return 0;
    }

    if (fstat(fileno(uploadfp), &file_stat) < 0) {
        fprintf(stderr, "Error fstat --> %s", strerror(errno));
        return 0;
    }



    sprintf(file_name_size, "%s %d", fileName,file_stat.st_size);
    printf("Sending file %s to %s \n",fileName,list->hostname);


    send(list->socketFd, file_name_size, sizeof(file_name_size),0);


    memset(send_buffer,0,SEND_BUFSIZE);
    int sentBytes;
    gettimeofday (&startTime, NULL);

    while((sentBytes = fread(send_buffer, sizeof(char), SEND_BUFSIZE, uploadfp)) > 0) {
        if(send(list->socketFd, send_buffer, sentBytes, 0) < 0) {
            fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fileName, errno);
            break;
        }
    }
    gettimeofday (&endTime, NULL);

    printf("Sent %s to %s\n", fileName,list->hostname);
    long timeTaken=((endTime.tv_sec - startTime.tv_sec)*1000000L+endTime.tv_usec) - startTime.tv_usec;
    printf("Time taken to upload file: %ld microseconds\n",timeTaken);
    long txRate = (((long)file_stat.st_size*8 )/ timeTaken);

    printf("TX Rate is :  %ld x 10^-6 Mbits/microsecs \n",txRate);
    return 1;

}

/*
* Function:  displayAll 
* --------------------
* Displays all peers connected to the server
*
*   list: List containing all peers connected to the Server
*   size: size of list          
*/


void displayAll(peer* list,int size)
{
    int i;
    printf("Client : Server has sent the updated peer list\n");
    printf("Hostname\t\tIP Address\t\tPort No.\n");
    for(i=0;i<size;i++)
    {
        printf("%s\t\t%s\t\t%d\n",list[i].hostname,list[i].IPAddress,list[i].port);
    }
}

/*
* Function:  reg 
* --------------------
* Register to the server
*
*
*   fd:             File descriptor used in connect
*   lPort:          Listening port of client     
*   client_IP_list: IP List
*   
*   returns:        1 if register was successful
*                   0 if register failed
*          
*/



int reg( int fd,char* lPort,node ** client_IP_list) {
    char* ip = parsedCommand[1];
    int port = atoi(parsedCommand[2]);
    struct in_addr ip_struct;
    struct sockaddr_in address;

    inet_aton(ip,&ip_struct);
    address.sin_addr=ip_struct;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
        //

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
        hints.ai_socktype = SOCK_STREAM;
       // getaddrinfo(ip, parsedCommand[2], &hints, &res);
        // getaddrinfo causing memory corruption, try to fix

        //



        if(connect(fd,(struct sockaddr*)&address,sizeof(address))==0) {
            printf("Registration successful! \n");
            send(fd,lPort,10,0);
            push(address,fd,port,client_IP_list);
            if(!res)
                freeaddrinfo(res);

            return 1;

        }
        else {
            perror("Client");
            if(!res)
                freeaddrinfo(res);
            return 0;
        }

}

/*
* Function:  con 
* --------------------
* Connect to another client
*
*
*   client_IP_list: IP List
*   allPeers:    Peers connected to server
*   size:           size of allPeers
*   listeningPort:  Listening port of client     
*   
*   returns:        sockFD if connection was successful
*                   0 if connection failed/error encountered
*          
*/

int con(node* client_IP_list,peer* allPeers,int size, char* listeningPort)
{

    char* peerIP = parsedCommand[1];
    int peerPort = atoi(parsedCommand[2]);
    node *curr=client_IP_list;


    char hname[256];
    if (gethostname(hname, sizeof(hname)) < 0) {
        perror("gethostname");
        return 0;
    }



    if(!client_IP_list) {
        printf("Client : I am not registered to the server. I am not allowed to connect to peers.\n");
        return 0;
    }
    else if(strcmp(peerIP,client_IP_list->IPAddress)==0) {
        printf("Client : You do not connect to a server. You can only register to a server!\n");
        return 0;
    }
    else if(strcmp(peerIP,client_IP_list->hostname)==0) {
        printf("Client : You do not connect to a server. You can only register to a server!\n");
        return 0;
    }
    else if(strcmp(peerIP,hname)==0) {
        printf("Client: I cannot connect to myself!\n");
        return 0;
    }

        //Causing memorry corruption for some reason.
    else if(strcmp(peerIP,myIP)==0) {
      printf("Client: I cannot connect to myself!\n");
            //free(machineIP);
      return 0;
  }
        //free(machineIP);

  else if( isValid(peerIP,allPeers,size) == 0 ){
    printf("Client: No such IP Address is registered to the server!\n");
    return 0;

}



while(curr)
{
    if(strcmp(peerIP,curr->IPAddress)==0)
    {
        printf("Client : I am already connected to %s \n",curr->hostname);
        return 0;
    }
    if(strcmp(peerIP,curr->hostname)==0)
    {
        printf("Client : I am already connected to %s \n",curr->hostname);
        return 0;
    }
    curr = curr->next;
}



struct in_addr peerIP_struct;
struct sockaddr_in peerDetails;
int sockFD;

inet_aton(peerIP,&peerIP_struct);
peerDetails.sin_addr = peerIP_struct;
peerDetails.sin_family = AF_INET;
peerDetails.sin_port = htons(peerPort);
sockFD=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
if(sockFD == -1)
{
    printf("Client: Not able to create socket!\n");
    close(sockFD);

    return 0;
}
        //

struct addrinfo hints, *res;
memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
        hints.ai_socktype = SOCK_STREAM;
        // getaddrinfo(peerIP, parsedCommand[2], &hints, &res);

        //


        if(connect(sockFD,(struct sockaddr*)&peerDetails,sizeof(peerDetails))<0)
        {    

            perror("Client");
            close(sockFD);
            if(!res)
                freeaddrinfo(res);

            return 0;
        }

        printf("Client : Connected to IPAddress %s \n",peerIP);

        send(sockFD,listeningPort,10,0);
        push(peerDetails,sockFD,peerPort,&client_IP_list);

        if(!res)
            freeaddrinfo(res);

        return sockFD;

}
/*
* Function:  terminate 
* --------------------
* Terminate connection to a client
*
*   list:           IP List
*
*   returns:        File descriptor of terminated connection if termination successful
*                   -1 if termination failed
*          
*/

int terminate(node** list){
    int connID = atoi(parsedCommand[1]);
        /*
        if(connID==1){
            printf("Cannot terminate connection with server! \n");
            return -1;
        }
        */
        int i;
        int fd;

        node* curr = *list;
        i=1;

        while(curr){
            if(i==connID){
                if(strcmp(curr->hostname,"timberlake.cse.buffalo.edu")==0){
                    printf("Cannot terminate connection with server! \n");


                    return -1;
                }
                fd=curr->socketFd;
                shutdown(fd,SHUT_RDWR);
                if(close(fd)== 0){ //Termiate successful
                    delete(curr->IPAddress,curr->port,list);
                    printf("Terminated connection to %s \n",curr->hostname);
                    return fd;
                }
            }
            i++;
            curr=(node*) curr->next;


        }

        return -1;
}

/*
* Function:  client 
* --------------------
* Client
*   port:       Port number to which client should be bound
*   
*   returns:    1 if client was shutdown
*               0 if some error was encountered
*          
*/


int client(char* port)
{
    //printf("packet size is %d\n",SEND_BUFSIZE);

    int completed=0;

    int noOfFilesToSync=0;
    int syncMode=0;
    

    node *client_IP_list = NULL;

    char msg[10000];


    int cmdNo; 

    char* tokens[3]; 
    char fileName[500];
    FILE *uploadfp=NULL;
    FILE *downloadfp=NULL;

    //sockaddr_in

    struct sockaddr_in myIPAddress,peerIPAddress;

    //socklen_t
    socklen_t peerIPAddrLen = sizeof(peerIPAddress);



    int noOfConnections = 0;

    int numBytes; // Number of bytes read using recv function

    //Socket file descriptors

    int clientSocket;
    int listenerSocket;
    int newSocket; 
    int selectedSocket;

    //FD
    fd_set read_fds,read_fdsCopy,write_fds,write_fdsCopy;
    int fdMax;

    //Initialize FD
    FD_ZERO(&read_fds);
    FD_ZERO(&read_fdsCopy);
    FD_ZERO(&write_fds); 
    FD_ZERO(&write_fdsCopy);


    printf("Client is up!\n");

    peer* allPeers;
    node* list;
    int countOfPeers;

    // Listener Socket
    listenerSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    printf("Client created \n");

    if(listenerSocket == -1) {
        printf("Client: \n");
    }
    
    int reuse=1;
    setsockopt(listenerSocket,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));

    myIPAddress.sin_addr.s_addr = INADDR_ANY;
    myIPAddress.sin_family = AF_INET;
    myIPAddress.sin_port = htons(atoi(port));

    if(bind(listenerSocket,(struct sockaddr*)&myIPAddress,sizeof(myIPAddress))== -1)
    {
        printf("Listener : Binding failed\n");
        close(listenerSocket); 
    }
    printf("Listener has been bound to port %d \n",atoi(port));


    

    listen(listenerSocket,4);
    printf("Listening on port %d \n",atoi(port));



    fdMax = listenerSocket;


    // Client Socket
    clientSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(clientSocket==-1)
    {
        printf("Client : Could not create socket to connect to server/n");
    }

    // SetFD
    FD_SET(STDIN_FILENO,&read_fds);
    FD_SET(listenerSocket,&read_fds);



    while(1)
    {


        read_fdsCopy = read_fds;
        write_fdsCopy = write_fds;

        if (select(fdMax+1, &read_fdsCopy, &write_fdsCopy, NULL, NULL) == -1)
        {
            perror("Client");
            continue;
        }

        for(selectedSocket = 0;selectedSocket<=fdMax;selectedSocket++)
        {
            if(FD_ISSET(selectedSocket,&read_fdsCopy))
            {

                if(selectedSocket == listenerSocket) // This is a connection request

                {

                    if(noOfConnections == 4)
                    {
                        printf("Client: Connection limit exceeded!\n");
                        continue;
                    }
                    memset(&peerIPAddress, 0, sizeof(peerIPAddrLen));
                    if ((newSocket = accept(listenerSocket,(struct sockaddr*)&peerIPAddress,&peerIPAddrLen))==-1)
                    {
                        printf("Client : Accept failed.\n");
                    }
                    memset(msg,0,5000);
                    // Receive clients listening port number
                    numBytes = recv(newSocket,msg,10,0);
                    printf("Connection request from %s : %d \n",inet_ntoa(peerIPAddress.sin_addr),atoi(msg));
                    

                    push(peerIPAddress,newSocket,atoi(msg),&client_IP_list);
                    noOfConnections++;
                    FD_SET(newSocket,&read_fds); // Add new socket to FD

                    printf("Connection established.Yay!\n");
                    fdMax = getMaxFD(client_IP_list);


                }
                else if(selectedSocket ==  clientSocket) // Data from server
                {
                    /* What data ?
                    1. Server shutdown
                    2. Recv function error
                    3. Peer list
                    */

                    numBytes = recv(selectedSocket, msg, 5000,0);
                    char command2[4];
                        memcpy(command2,msg,4); // Extract first 3 characters from incoming data


                    if(numBytes == 0) { // Server has shutdown 
                        printf("Client: Server terminated my connection/shutdown\n");
                        noOfConnections--;
                        printf("Closing all connections\n"); //TODO
                        client_IP_list=NULL;
                        FD_CLR(selectedSocket,&read_fds);
                        close(selectedSocket); // clientSocket closed
                        clientSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); // recreate 
                    }
                    else if(numBytes<0) { // Recv function error
                        perror("Client");
                    }
                    
                    else if(!(strcmp(command2,"SYNC")) || (!(strcmp(command2,"sync")))) { // Check if it is GET command

                            // Extract list of all peers who finished SYNC.

                            //Send text file named after me to all connected peers

                        syncMode=1;        
                        char * file;
                        char * dir="/local/Fall_2015/akannan4/";
                        char * ext=".txt";





                        node *curr=client_IP_list;


                        int i=1;
                        
                        char connID[15];
                        char** tokens = (char**) malloc(5*sizeof(char*));


                        
                           curr=curr->next; // skip server
                           noOfFilesToSync=noOfConnections-1;
                           if(curr){
                            i=2;
                            

                            sprintf(connID, "%d", i);

                            file = strtok(curr->hostname,".");

                            char * filename = (char *) malloc(1 + strlen(dir)+ strlen(file)+ strlen(ext) );
                            strcpy(filename, dir);
                            strcat(filename, file);
                            strcat(filename, ext);

                            printf("Start %s %s\n",filename,printTime());





                            tokens[0]="GET";
                            tokens[1]=connID;
                            tokens[2]=filename;

                            




                            i++;
                            parsedCommand=tokens;

                            get(client_IP_list);
                            completed++;
                        }
                        else{
                            syncMode=0;
                        }



                        


                    }

                                           else { // Peer list

                                            if(!allPeers) free(allPeers);
                                            countOfPeers = numBytes/sizeof(peer);

                                            allPeers = (peer*)malloc(sizeof(peer)*(countOfPeers));
                                            memcpy(allPeers,msg,sizeof(msg));
                                            displayAll(allPeers,countOfPeers);

                                            

                                            memset(msg,0,sizeof(msg));
                                        }
                                    }
                else if(selectedSocket == 0) // Data from STDIN 
                {


                    memset(msg,0,sizeof(msg)); // clear msg array

                    if ((numBytes = read(selectedSocket, msg, sizeof(msg))) <= 0)
                        perror("Client");                    
                    else
                    {

                        cmdNo = parse(msg);

                        int fd;
                        int error;
                        int retval;
                        peer peers[1];

                        switch(cmdNo)
                        {
                            case 0: //HELP
                            help();
                            break;

                            case 1: //CREATOR
                            creator();
                            break;

                            case 2: //DISPLAY
                            display(port,1);
                          //  free(machineIP);
                            break;

                            case 3: //REGISTER
                           // clientSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

                            if(reg(clientSocket,port,&client_IP_list)){
                                noOfConnections++;
                                if(fdMax < clientSocket)
                                    fdMax = clientSocket;
                                
                                FD_SET(clientSocket,&read_fds);
                            }
                            break;

                            case 4: //CONNECT
                            if(noOfConnections >= 4) { // why 4? 3 connnections to peers + one connection to server!
                                printf("Client : Connection limit exceeded!\n");
                                
                            }
                            else {
                                fd=con(client_IP_list,allPeers,countOfPeers,port);
                                if(fd>0){
                                    fdMax=getMaxFD(client_IP_list);
                                    FD_SET(fd,&read_fds);
                                    noOfConnections++;
                                }
                            }
                            break;
                            
                            case 5: // LIST
                            displayList(client_IP_list);    
                            break;

                            case 6: // TERMINATE
                            fd=terminate(&client_IP_list);
                            if(fd!=-1){
                                FD_CLR(fd,&read_fds);
                                FD_CLR(fd,&write_fds);

                                fdMax = getMaxFD(client_IP_list);
                                noOfConnections--;
                                if(noOfConnections==0){
                                    client_IP_list=NULL;
                                }

                            }
                            else{

                                printf("Invalid connection ID \n");

                            }

                            break;

                            case 7: // QUIT
                            quit(&client_IP_list);
                            shutdown(listenerSocket,SHUT_RDWR);
                            close(listenerSocket);
                            shutdown(clientSocket,SHUT_RDWR);

                            close(clientSocket);
                            return 1;
                            break;

                            case 8: // GET
                            get(client_IP_list);
                            break;
                            case 9: // PUT
                            put(client_IP_list);
                            break;
                            case 10: //SYNC
                            syncFiles(client_IP_list);
                            
                            break;

                            default:

                            break;
                        }

                    }

                }

                else{ // Data from another peer.
                    /* What data ?
                    1. Peer connection closed.
                    2. GET command from peer
                    3. Incoming file from peer.
                    */
                    numBytes = recv(selectedSocket, msg, sizeof(msg), 0);

                    if (numBytes == 0) { // Peer connection closed.

                        //So which peer ?
                        memset(&peerIPAddress,0,peerIPAddrLen);
                        if(getpeername(selectedSocket,(struct sockaddr*)&peerIPAddress,&peerIPAddrLen)<0) {
                            perror("Client\n");
                            return 1;
                        }

                        char* IPAdd = (char*)malloc(sizeof(char)*INET6_ADDRSTRLEN);
                        inet_ntop(AF_INET, &(peerIPAddress.sin_addr), IPAdd, INET6_ADDRSTRLEN);

                        printf("Client %s has closed the connection\n",IPAdd);

                        delete(IPAdd,ntohs(ntohs(peerIPAddress.sin_port)),&client_IP_list);
                        noOfConnections--;

                        FD_CLR(selectedSocket, &read_fds);
                        fdMax = getMaxFD(client_IP_list);
                        close(selectedSocket);

                    }
                    else if(numBytes<0) { // Recv function error
                        perror("Client");
                    }


                    else {
                        /*
                        1. Check if data is a GET command
                        2. Check if data is a file
                        */

                        char command[3];
                        
                        memcpy(command,msg,3); // Extract first 3 characters from incoming data

                        char* host=getHostFromSock(selectedSocket,client_IP_list);
                        if(!(strcmp(command,"GET")) || (!(strcmp(command,"get")))  ) { // Check if it is GET command
                         
                            char path[1024];
                            getcwd(path,sizeof(path));

                            int tokenCount = 0;
                            char file_name_size[256];
                            struct stat file_stat;
                            int sent_bytes;
                            tokens[tokenCount] = strtok(msg," ");

                            while(tokens[tokenCount])
                            {
                                tokens[++tokenCount]=strtok(NULL," ");
                            }

                            strcpy(fileName, tokens[2]);
                            strcat(path,"/");
                            strcat(path,fileName);
                            printf("%s\n",path);

                            printf("File '%s' requested by '%s' \n",fileName,host);
                            //uploadfp=fopen(path, "r");
                            uploadfp=fopen(path, "r");
                            if(uploadfp==NULL) {
                                printf("The requested file '%s' does not exist\n",fileName);
                                continue;
                            }

                            if (fstat(fileno(uploadfp), &file_stat) < 0) {
                                fprintf(stderr, "Error in fstat call --> %s", strerror(errno));
                                continue;
                            }

                            sprintf(file_name_size, "%s %d", fileName,file_stat.st_size);
                            printf("Sending file '%s' to '%s' \n",fileName,host);


                            send(selectedSocket, file_name_size, sizeof(file_name_size),0); // send file name and size first 


                            char send_buffer[SEND_BUFSIZE];
                            memset(send_buffer,0,SEND_BUFSIZE);
                            int sentBytes;
                            gettimeofday (&startTime, NULL);

                            while((sentBytes = fread(send_buffer, sizeof(char), SEND_BUFSIZE, uploadfp)) > 0) { // start sending file contents
                                if(send(selectedSocket, send_buffer, sentBytes, 0) < 0) {
                                    fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fileName, errno);
                                    break;
                                }
                            }
                            gettimeofday (&endTime, NULL);

                            
                            printf("Sent '%s' of size '%d' bytes to '%s' \n", fileName,file_stat.st_size,host);
                            long timeTaken=((endTime.tv_sec - startTime.tv_sec)*1000000L+endTime.tv_usec) - startTime.tv_usec;

                            printf("Time taken to upload file: %ld microseconds\n",timeTaken);
                            long txRate = (((long)file_stat.st_size*8 )/ timeTaken);

                            printf("TX Rate is :  %ld x 10^-6 Mbits/microsecs \n",txRate);

                            
                        }







                        else { // Data is a file! Let's save it!   


                            int tokenCount = 0;
                        int file_size;
                        int remain_data;
                        char* file_name;
                        char receive_buffer[RECV_BUFSIZE];


                        tokens[tokenCount] = strtok(msg," ");


                        while(tokens[tokenCount]) {
                            tokens[++tokenCount]=strtok(NULL," ");
                        }
                        file_name=tokens[0];
                        file_size=atoi(tokens[1]);
                        remain_data = file_size;
                        if(noOfFilesToSync==0){

                            printf("Receiving file '%s' of size %d bytes from '%s' \n",file_name,file_size,host);
                        }

                        downloadfp = fopen(file_name, "w");
                        if (downloadfp == NULL) {
                            fprintf(stderr, "Open failed. %s\n", strerror(errno));

                        }

                        int readBytes = 0;

                        memset(receive_buffer,0,RECV_BUFSIZE);

                        gettimeofday (&startTime, NULL);

                        while(remain_data >0) {

                            readBytes = recv(selectedSocket, receive_buffer, RECV_BUFSIZE, 0);



                            int write_sz = fwrite(receive_buffer, sizeof(char), readBytes, downloadfp);
                            fflush(downloadfp);

                            remain_data=remain_data-readBytes;



                        }
                        gettimeofday (&endTime, NULL); // how to handle this for PUT ?
                        if(noOfFilesToSync>0){
                            char connID[15];
                            printf("End %s%s\n",file_name,printTime());
                            noOfFilesToSync--;
                            node *curr=client_IP_list;
                            curr=curr->next; //skip server
                            
                            int i;
                            for(i=0;i<completed;i++){
                                curr=curr->next;


                            }
                            sprintf(connID,"%d",i+2);
                            char * file;
                            char * dir="/local/Fall_2015/akannan4/";
                            char * ext=".txt";
                            file = strtok(curr->hostname,".");

                            char * filename = (char *) malloc(1 + strlen(dir)+ strlen(file)+ strlen(ext) );
                            strcpy(filename, dir);
                            strcat(filename, file);
                            strcat(filename, ext);

                            printf("Start %s %s\n",filename,printTime());
                            
                            tokens[0]="GET";
                            tokens[1]=connID;
                            tokens[2]=filename;
                            i++;
                            parsedCommand=tokens;

                            get(client_IP_list);
                            completed++;




                        }
                        else{

                            printf("Received file '%s' of size %d bytes from '%s' \n",file_name,file_size,host);
                            long timeTaken=((endTime.tv_sec - startTime.tv_sec)*1000000L+endTime.tv_usec) - startTime.tv_usec;
                            printf("Time taken to download file: %ld microseconds\n",timeTaken);
                            long rxRate = (((long)file_size*8 )/ timeTaken);

                            printf("RX Rate is :  %ld  x 10^-6 Mbits/microsecs\n",rxRate);
                            


                        }
                        if(noOfFilesToSync==0 && syncMode==1){
                            endSyncFiles(client_IP_list);
                            completed=0;
                            syncMode=0;
                        }

                        


                        
                        
                        

                    }

                        memset(msg,0,5000); // reset read buffer

                    }

                }
            }
        }// End For

    } // End While
} 


