/*This file is for Client and processing all its commands*/
#include <fstream>
#include "Client.h"
#include <stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include <sys/statvfs.h>
#include "Defines.h"

//Set Port number
void Client::SetPort(int portNum)
{
    portNumber = portNum;
}

//Get port number
int Client::GetPortNumber()
{
    return portNumber;
}

//Get IP from Socket structure
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
}

//To check the amount of disk space left while some other peer is uploading into our disk
// We don't want other peers to fill up our disk
bool Client::DiskSpaceLeft(long long int fileSize)
{
    struct statvfs buf;

    int ret = statvfs(".",&buf);
    if(ret != 0)
    {
        cout<<"Get disk space left error\n";
        return false;
    }

    long long int sz = buf.f_bsize*buf.f_bfree;
 
    if(sz < fileSize)
    {
        cout<<"No space left to accomodate the receiving file\n";
        return false;
    }
    return true;
}

//This API is used when file download is done.
// Every time a file download is started, we maintain its parameters like the amount
//of bytes read and the number of bytes still left to read.
// It also tracks multiple downloads using the file descriptor
void Client::AddFileReadDetails(int fd,long long int bytesRead,long long int bytesLeft,char *fileName)
{
    FileReadDetails *tempStruct = firstReadFile;

	if(firstReadFile == NULL)
	{
		dout<<"No File Details Present at all\n";

		FileReadDetails *addDetails = new FileReadDetails;
		addDetails->fd = fd;
		addDetails->bytesRead = bytesRead;
		addDetails->bytesLeft = bytesLeft;
		strcpy(addDetails->fileName,fileName);
		addDetails->next = NULL;
		firstReadFile = addDetails;
		return;
	}

	FileReadDetails *prevDetails;

	while(tempStruct != NULL && tempStruct->fd != fd)
	{
		prevDetails = tempStruct;
		tempStruct = tempStruct->next;

	}

	if(tempStruct == NULL)
	{
		if(bytesLeft != 0)
		{
			dout<<"File Details Not added, add now\n";

			FileReadDetails *addDetails = new FileReadDetails;
		        addDetails->fd = fd;
		        addDetails->bytesRead = bytesRead;
		        addDetails->bytesLeft = bytesLeft;
		        strcpy(addDetails->fileName,fileName);
		        addDetails->next = NULL;
			prevDetails->next = addDetails;
		}
	}
	else
	{
		if(bytesLeft != 0)
		{
            		dout<<"BytesLeft in read "<<bytesLeft<<"\n";
			tempStruct->bytesLeft = bytesLeft;
			tempStruct->bytesRead = bytesRead;
		}
		else
		{
			if(tempStruct != NULL && tempStruct != firstReadFile)
			{
				prevDetails->next = tempStruct->next;
			}
			else if(tempStruct == firstReadFile)
			{
				firstReadFile = firstReadFile->next;
			}
			delete tempStruct;

		}
	}
}

//This API is used during file downloading. This is used to get the details that were saved in the previous 
//partial download like number of bytes downloaded and number of bytes still left
int Client::GetFileReadDetails(int fd,struct FileReadDetails *fDetails)
{
	FileReadDetails *tempStruct = firstReadFile;
   
	while(tempStruct != NULL && tempStruct->fd != fd)
	{
		tempStruct = tempStruct->next;
	}

	if(tempStruct == NULL)
	{
		return -1;
	}

	fDetails->bytesLeft = tempStruct->bytesLeft;
	fDetails->bytesRead = tempStruct->bytesRead;
	strcpy(fDetails->fileName,tempStruct->fileName);
    	return 0;
}


//This API is used when file upload is done.
// Every time a file upload is started, we maintain its parameters like the amount
//of bytes written(sent) and the number of bytes still left to write(to send).
// It also tracks multiple uploads using the file descriptor
void Client::AddFileWriteDetails(int fd,long long int bToWrite,long long int bWritten,char *fName)
{
	FileWriteDetails *tempStruct = firstWriteFile;

	if(firstWriteFile == NULL)
	{
		FileWriteDetails *addDetails = new FileWriteDetails;
		addDetails->fd = fd;
		addDetails->bytesToWrite = bToWrite;
		addDetails->bytesWritten = bWritten;
		strcpy(addDetails->fileName,fName);
		addDetails->next = NULL;
		firstWriteFile = addDetails;
		return;
	}

	FileWriteDetails *prevDetails;

	while(tempStruct != NULL && tempStruct->fd != fd)
	{
		prevDetails = tempStruct;
		tempStruct = tempStruct->next;

	}

	if(tempStruct == NULL)
	{
		if(bToWrite != 0)
		{
			dout<<"File Details Not added, add now\n";
			FileWriteDetails *addDetails = new FileWriteDetails;
			addDetails->fd = fd;
			addDetails->bytesToWrite = bToWrite;
			addDetails->bytesWritten = bWritten;
			strcpy(addDetails->fileName,fName);
			addDetails->next = NULL;
			prevDetails->next = addDetails;
		}
	}
	else
	{
		if(bToWrite != 0)
		{
			tempStruct->bytesToWrite = bToWrite;
			tempStruct->bytesWritten = bWritten;
		}
		else
		{
			if(tempStruct != NULL && tempStruct != firstWriteFile)
			{
				prevDetails->next = tempStruct->next;
			}
			else if(tempStruct == firstWriteFile)
			{
				firstWriteFile = firstWriteFile->next;
			}
			delete tempStruct;

		}
	}
}


//This API is used during file uploading. This is used to get the details that were saved in the previous 
//partial upload like number of bytes uploaded and number of bytes still left to upload
int Client::GetFileWriteDetails(int fd,struct FileWriteDetails *fDetails)
{
	FileWriteDetails *tempStruct = firstWriteFile;
   
	while(tempStruct != NULL && tempStruct->fd != fd)
	{
		tempStruct = tempStruct->next;
	}

	if(tempStruct == NULL)
	{
		return -1;
	}

	fDetails->bytesToWrite = tempStruct->bytesToWrite;
	fDetails->bytesWritten = tempStruct->bytesWritten;
	strcpy(fDetails->fileName,tempStruct->fileName);
	return 0;

}


//This API is used to check if the file upload is completed or not
// If the file upload is completed then its file descriptor won't be availble in the 
//list of structures containing the file write details
bool Client::WriteComplete(int fd)
{
	FileWriteDetails *tempStruct = firstWriteFile;

	while(tempStruct != NULL)
	{
		if(tempStruct->fd == fd)
		{
			return false;
		}
		tempStruct = tempStruct->next;
	}
	return true;

}

//This API is executed when GET command is inputted on client
// The connectionID is the one obtained from LIST command
//fName is the file we would like to download
void Client::ExecuteGET(int connectionID, char *fName)
{

    ClientList *tempList = cList;

    dout<<"Execute GET \n";

    for(int i = 1; tempList != NULL ; i++)
    {
        if(i == connectionID)
        {
            break;
        }
        tempList = tempList->nextList;
    }

    if(tempList == NULL)
    {
        cout<<"GET Command Error!!! Connection ID not found\n";
        return;
    }

    int fd = tempList->fd;

    if(CheckClientDetailsFlag(fd))
    {
        cout<<"Error!!! Already one download going on with the same client\n";
        return;
    }

    char buf[BUFF_LEN];

    CommandDetails *cmdDetails = new CommandDetails;
    cmdDetails->commandType = GET;
    cmdDetails->len = 48;

    memcpy((void*)buf,cmdDetails,sizeof(CommandDetails));

    FileNameDetails *_fName = new FileNameDetails;


     char *slashPosition = strrchr(fName,'/');
     char _fileName[64] = {0,};
     if(slashPosition != NULL)
     {
        strcpy(_fileName,fName+(slashPosition-fName+1));
     }
     else
     {
        strcpy(_fileName,fName);
     }


    strcpy(_fName->fileName,_fileName);

    memcpy((void*)(buf+sizeof(CommandDetails)),_fName->fileName,sizeof(FileNameDetails));
    int n = 0;
    n = send(fd,buf, sizeof(CommandDetails)+sizeof(FileNameDetails), 0);
    if (n == -1)
    {
        perror("Send Error\n");
    }
    dout<<n<<" bytes sent from GET command intially\n";
    delete cmdDetails;
    delete _fName;
}

//This API is executed when PUT command is inputted on client
// The connectionID is the one obtained from LIST command
//fName is the command we would like to upload to other peer
void Client::ExecutePUT(int connectionID, char *fName)
{
    ClientList *tempList = cList;

    for(int i = 1; tempList != NULL ; i++)
    {
        if(i == connectionID)
        {
            break;
        }
        tempList = tempList->nextList;
    }

    if(tempList == NULL)
    {
        cout<<"PUT Command Error!!! Connection ID not found\n";
        return;
    }

    int fd = tempList->fd;

    if(!WriteComplete(fd))
    {
        cout<<"Error!!! Already one upload going on with the same client\n\n";
        return;
    }

    dout<<"Execute PUT with fd : "<<fd<<"\n";

    _ExecutePUT(fd,fName);
}

//This is the actual API that is to be called for uploading
//above ExecutePUT() is like a wrapper API
void Client::_ExecutePUT(int sockfd, char *filePath)
{
    FD_SET(sockfd,&write_master);

    CommandDetails *cmdDetails = new CommandDetails;
    cmdDetails->commandType = PUT;

    char _buff[BUFF_LEN];
    ifstream FileToSend(filePath, ios::in | ios::binary);
    if(!FileToSend.is_open())
    {
        cout<<"File not found/Open Error in read, abort\n";
        cmdDetails->len = -1;
        memcpy((void*)_buff,cmdDetails,sizeof(CommandDetails));
        send(sockfd,_buff,sizeof(CommandDetails),0);
        return;
    }

    cout<<"Upload Started for the file "<<filePath<<"\n";

     char *slashPosition = strrchr(filePath,'/');
     char _fileName[64] = {0,};
     if(slashPosition != NULL)
     {
        strcpy(_fileName,filePath+(slashPosition-filePath+1));
     }
     else
     {
        strcpy(_fileName,filePath);
     }

	FileToSend.seekg(0,ios::end);
	long long int _end = FileToSend.tellg();
	FileToSend.seekg(0,ios::beg);

	cmdDetails->len = _end;
    	memcpy((void*)_buff,cmdDetails,sizeof(CommandDetails));

        FileNameDetails *fName = new FileNameDetails;
        strcpy(fName->fileName,_fileName);
        memcpy((void*)(_buff+sizeof(CommandDetails)),fName,sizeof(FileNameDetails));

	long long int bytesToWrite = _end;
	long long int bytesWritten = 0;


        FileToSend.read(_buff+COMMAND_LEN,BUFF_LEN-COMMAND_LEN);

        long long int total = 0;
        long long int bytesleft = MIN(BUFF_LEN,bytesToWrite+COMMAND_LEN);
        long long int limit = bytesleft;
        long long int n;
        while(total < limit)
        {
            n = send(sockfd,_buff+total, bytesleft, 0);
            if (n == -1)
            {
                perror("Send Error\n");
                break;
            }
            total += n;
            bytesleft -= n;
            bytesWritten += n;
    }

    bytesWritten-=COMMAND_LEN;
    bytesToWrite-=(total-COMMAND_LEN);

    if(bytesToWrite > 0)
    {
        AddFileWriteDetails(sockfd,bytesToWrite,bytesWritten,filePath);
    }
    else
    {
        cout<<"Upload completed for the file "<<filePath<<" in one go to the host : "<<GetHostName(sockfd)<<"\n";
    }

    FileToSend.close();
    delete cmdDetails;
}

//This API is executed to terminate all the peer connections and exit
void Client::ExecuteQuit()
{
    ClientList *tempList = cList;
    ClientList *prevList;
    while(tempList != NULL)
    {
        dout<<"FD : "<<tempList->fd<<"\n";
        FD_CLR(tempList->fd, &read_master);
        FD_CLR(tempList->fd, &read_fds);
        close(tempList->fd);
        prevList = tempList;
        tempList = tempList->nextList;

        delete prevList;
    }
    cout<<"Client Quitting, bye-bye\n";
    exit(0);
}


//This API is executed to terminate the peer connection with connection ID  = connectionID
void Client::ExecuteTerminate(int connectionID)
{

    ClientList *tempList = cList;
    ClientList *prevList;
    cout<<"Terminating Connection ID "<<connectionID<<"\n";
    if(connectionID == 1 && cList != NULL)
    {
        cList = cList->nextList;
        FD_CLR(tempList->fd, &read_master);
        FD_CLR(tempList->fd, &read_fds);
        close(tempList->fd);
        if(serverFd != -1)
        {
            isServerConnected = false;
            peerConnections--;
            serverFd = -1;
        }
        delete tempList;
        return;
    }
    else
    {
        int i = 1;
        while(tempList != NULL)
        {
            if(i == connectionID)
            {
                peerConnections--;
                FD_CLR(tempList->fd, &read_master);
                FD_CLR(tempList->fd, &read_fds);
                close(tempList->fd);
                prevList->nextList = tempList->nextList;
                delete tempList;
                return;
            }
            i++;
            prevList = tempList;
            tempList = tempList->nextList;
        }
    }

    cout<<"Connection Id not found\n";

}


//Upon executing this command all the peer connections with this client shall be displayed
void Client::ExecuteList()
{
    ClientList *tempList = cList;

    cout<<"Id\tHostname\t\t\tIP address\tPort No.\n";

    for(int i = 1; tempList != NULL ; i++)
    {
        cout<<i<<"\t"<<tempList->hostName<<"\t"<<tempList->IP<<"\t"<<tempList->portNumber<<"\n";
        tempList = tempList->nextList;
    }
}

//This API is used to connect with other registeted clients with a given IP and port number
void Client::ExecuteConnect(char *destIP, int portNum)
{
   dout<<"Client : ExecuteConnect "<<__LINE__<<"\n";
   struct addrinfo hints, *servinfo, *p;
   int ret = 0;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   char buf[BUFF_LEN];
   int sockfd = 0;

   bool isIPName = false;

   char *tempStr = new char[strlen(destIP)+1];
   strncpy(tempStr,destIP,strlen(destIP)+1);

   isIPName = IdentifyIPorHostName(tempStr);

   if(!isIPName)
   {
        struct hostent *temp;
        if((temp = gethostbyname(tempStr)) != NULL)
        {
            struct in_addr **addr_list;
            addr_list = (struct in_addr **)temp->h_addr_list;
            memset(destIP,0,strlen(destIP)+1);
            strcpy(destIP,inet_ntoa(*addr_list[0]));
        }
        else
        {
             cout<<"Error in getting the IP\n";
             return;
        }
   }

   delete tempStr;

   if(isServerConnected == false)
   {
     cout<<"Server is not connected, can't connect to any client\n";
     return;
   }

   if(ValidClientIP(destIP,portNum) == false)
   {
        cout<<"Connect Failed\n";
        return;
   }

   if(peerConnections >= MAX_CONNECTIONS)
   {
        cout<<"Peer Connections Limit exceeded , can't establish this connection\n";
        return;
   }


    char portTest[6] = {0,};

    HelperFunctions::PortNumToString(portNum,portTest);

    if ((ret = getaddrinfo(destIP,portTest,&hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    }

    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }
        dout<<"Sockfd : "<<sockfd<<"\n";
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        CommandDetails *_cmdDetails = new CommandDetails;
        _cmdDetails->commandType = 5;
        _cmdDetails->len = sizeof(ClientList);

        memcpy((void*)buf,(void*)_cmdDetails,sizeof(CommandDetails));

        ClientList *tempList = new ClientList;
        int len = strlen(hostIP);
        strncpy(tempList->IP,hostIP,len);
        tempList->IP[len] = '\0';
        tempList->portNumber = GetPortNumber();
        int hNamelen = strlen(hostName);
        strncpy(tempList->hostName,hostName,hNamelen);
        tempList->hostName[hNamelen] = '\0';

        tempList->nextList = NULL;

        memcpy((void*)(buf+sizeof(CommandDetails)),(void*)tempList,sizeof(ClientList));

        send(sockfd, buf, sizeof(ClientList)+sizeof(CommandDetails), 0);


        FD_SET(sockfd, &read_master);
        if (sockfd > fdmax)
        {
            fdmax = sockfd;
        }

        AddClientToList(destIP,portNum,sockfd);

        SetClientReceivingFlag(sockfd,false);
        peerConnections++;
        if(peerConnections >= MAX_CONNECTIONS)
        {
            cout<<"PeerConnection limit reached, close the listening port\n";
            FD_CLR(listener, &read_master);
            close(listener);
            listener = -1;
        }
        cout<<"Succesfully connected to IP : "<<destIP<<" with Port Number : "<<portNum<<"\n";
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
    }

    freeaddrinfo(servinfo);
}

//API to register with the server
void Client::ExecuteRegister(char *serverIP, int portNum)
{
   dout<<"Client : ExecuteRegister "<<__LINE__<<"\n";
   struct addrinfo *servinfo, *p;
   int ret = 0;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   char buf[BUFF_LEN];
   int sockfd = 0;


   char portTest[6] = {0,};


   bool isIPName = false;

   if(serverFd != -1)
   {
        cout<<"Error!!! Client already registered to one server\n";
        return;
   }

   if(strcmp(serverIP,hostIP) == 0 && (portNum == portNumber))
   {
        cout<<"Error!!! Can't register to self\n";
        return;
   }


   char *tempStr = new char[strlen(serverIP)+1];
   strncpy(tempStr,serverIP,strlen(serverIP)+1);

   isIPName = IdentifyIPorHostName(tempStr);

   if(!isIPName)
   {
        struct hostent *temp;
        if((temp = gethostbyname(tempStr)) != NULL)
        {
            struct in_addr **addr_list;
            addr_list = (struct in_addr **)temp->h_addr_list;
            memset(serverIP,0,strlen(serverIP)+1);
            strcpy(serverIP,inet_ntoa(*addr_list[0]));
        //    cout<<"IP address : "<<serverIP<<"\n";
        //    cout<<"Host Name : "<<tempStr<<"\n";
        }
        else
        {
             cout<<"Error in getting the IP\n";
             return;
        }
   }

   delete tempStr;


    HelperFunctions::PortNumToString(portNum,portTest);

    if((ret = getaddrinfo(serverIP,portTest,&hints, &servinfo)) != 0)
    {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    }

    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }
        dout<<"Sockfd : "<<sockfd<<"\n";
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        ServerIPList *tempList = new ServerIPList;
        tempList->fd = sockfd;
        int len = strlen(hostIP);
        strncpy(tempList->IP,hostIP,len);
        tempList->IP[len] = '\0';
        tempList->portNumber = GetPortNumber();
        tempList->nextList = NULL;

        memcpy((void*)buf,(void*)tempList,sizeof(ServerIPList));

        send(sockfd, buf, sizeof(ServerIPList), 0);

        FD_SET(sockfd, &read_master);
        if (sockfd > fdmax)
        {
            fdmax = sockfd;
        }

        SetClientReceivingFlag(sockfd,false);
        AddClientToList(serverIP,portNum,sockfd);
        isServerConnected = true;
        serverFd = sockfd;
        cout<<"Succesfully Registered to server : "<<serverIP<<" with Port Number : "<<portNum<<"\n";
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
    }
    //inet_ntop(p->ai_family, &(((struct sockaddr_in*)sa)->sin_addr),s, sizeof(s));
  //  printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo);

}


//This is the main API for client
//The client processing is done here
//Socket for the client, binding and listening is done here
//Select API is used to maange multiple TCP connections and also 
// to take input from STDIN simultaneously
void Client::Manager()
{
    dout<<"Debug "<<__LINE__<<"\n";

    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    char buf[BUFF_LEN];
    long long int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    int i, j, rv;

    FD_ZERO(&read_master);
    FD_ZERO(&read_fds);
    dout<<"Debug "<<__LINE__<<"\n";
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    OpenListeningPort(hints);

    //Adding STDIN
    FD_SET(0, &read_master);

    FD_SET(listener, &read_master);
    fdmax = listener;

    for(;;)
    {
        read_fds = read_master;
        write_fds = write_master;
	    //Select() to handle multiple TCP connections
        if (select(fdmax+1, &read_fds, &write_fds, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        for(i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
		//To check if there is input from STDIN    
                if(i == 0)
                {
                    std::string strCommand;
                    getline(cin,strCommand);
                    char *commandTokens[3] = {NULL,NULL,NULL};
                    HelperFunctions::TokenizeCommand((char *)strCommand.c_str(),commandTokens);
                    HelperFunctions::ToLowerCase(commandTokens[0]);
                    dout<<commandTokens[0]<<endl;
                    ExecuteCommand(commandTokens,this);
                    if((strcmp(commandTokens[0],"terminate") == 0)&& peerConnections == 2)
                    {
                        OpenListeningPort(hints);
                        FD_SET(listener,&read_master);
                        if (listener > fdmax)
                        {
                            fdmax = listener;
                        }
                    }
                }
		//To check if there is a new connection    
                else if (i == listener)
                {
                    addrlen = sizeof(remoteaddr);
               
                    newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);

                    socklen_t len;
                    struct sockaddr_storage addr;
                    char ipstr[INET6_ADDRSTRLEN];
                    int _port;
                    len = sizeof addr;
                    getpeername(newfd, (struct sockaddr*)&addr, &len);

                    if (addr.ss_family == AF_INET)
                    {
                        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
                        _port = ntohs(s->sin_port);
                        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
                    }

                      dout<<"Port Number from other client : "<<_port<<"\n";
                      dout<<"IP : "<<ipstr<<"\n";

                    if (newfd == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        FD_SET(newfd, &read_master);
                        if (newfd > fdmax)
                        {
                            fdmax = newfd;
                        }
                       
                        inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr),\
                        remoteIP, INET_ADDRSTRLEN);
                        char tempIP[32] = "UnKnown";

                        SetClientReceivingFlag(newfd,false);
                        printf("New Client connection from %s on socket %d\n",remoteIP,newfd);
                        peerConnections++;
                        if(peerConnections >= MAX_CONNECTIONS)
                        {
                            cout<<"PeerConnection limit reached, close the listening port\n";
                            FD_CLR(listener, &read_master);
                            close(listener);
                            listener = -1;
                        }
                    }

                }
		//Else condition is executed for all other conditions like
		//PUT or GET or CONNECT command data from other peers or
		//SERVER IP list data from server    
                else
                {
                    memset(buf,0,BUFF_LEN);
             
                    if ((nbytes = recv(i,(void*)buf, sizeof(buf), 0)) <= 0)
                    {
                        if(nbytes == 0)
                        {
                            peerConnections--;
                            if(peerConnections == MAX_CONNECTIONS-1)
                            {
                                OpenListeningPort(hints);
                                FD_SET(listener,&read_master);
                                if (listener > fdmax)
                                {
                                    fdmax = listener;
                                }
                            }

                            ClientList *tempList = cList;
                            ClientList *prevList;

                            if(tempList->fd == i)
                            {
                                if(tempList->fd == serverFd)
                                {
                                    isServerConnected = false;
                                    serverFd = -1;
                                    ClearServerList();
                                    cout<<"Server got disconnected\n";
                                }
                                else
                                {
                                    cout<<"Client with IP : "<<tempList->IP<<" and Port Num : "<<tempList->portNumber\
                                        <<" got disconnected\n";
                                }
                                cList = cList->nextList;
                                FD_CLR(i, &read_master);
                                delete tempList;
                            }
                            else
                            {

                                while(tempList != NULL)
                                {
                                    if(tempList->fd == i)
                                    {
                                        cout<<"Client with IP : "<<tempList->IP<<" and Port Num : "<<tempList->portNumber\
                                        <<" got disconnected\n";
                                        break;
                                    }
                                    prevList = tempList;
                                    tempList = tempList->nextList;
                                }
                                if(tempList != NULL)
                                {
                                    prevList->nextList = tempList->nextList;
                                    FD_CLR(i, &read_master);
                                    delete tempList;
                                }
                            }
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &read_master);
                    }
                    else
                    {
                        if(i == serverFd)
                        {
                            ServerIPList *recvClientList = (ServerIPList*)buf;
                            PrintServerIPList(recvClientList);
                            UpdateServerList(recvClientList);
                        }
                        else //If some download is going on
                        {
                            if(CheckClientDetailsFlag(i) == true)
                            {
                                FileReadDetails *readDetails = new FileReadDetails;
                                if(GetFileReadDetails(i,readDetails) == -1)
                                {
                                    cout<<"Get Read Details Error\n";
                                    continue;
                                }
                                long long int bytesRead = readDetails->bytesRead;
                                long long int bytesLeft = readDetails->bytesLeft;

                                dout<<"Bytes Read before read operation : "<<bytesRead<<"\n";
                                dout<<"Bytes Left before read operation : "<<bytesLeft<<"\n";

                                char _buff[BUFF_LEN];
                                memcpy(_buff,buf,nbytes);

                                ofstream fileToWrite(readDetails->fileName, ios::out | ios::app | ios::binary);

                                if(!fileToWrite.is_open())
                                {
                                    cout<<"File not found/ Open Error in read, abort\n";
                                    continue;
                                }

                                fileToWrite.write(_buff,nbytes);

                                bytesRead += nbytes;
                                bytesLeft -= nbytes;
                                AddFileReadDetails(i,bytesRead,bytesLeft,readDetails->fileName);
                                fileToWrite.close();

                                dout<<"Bytes Read after read operation : "<<bytesRead<<"\n";
                                dout<<"Bytes Left after read operation : "<<bytesLeft<<"\n";

                                if(bytesLeft <= 0)
                                {
                                    cout<<"Download Completed for the file : "<<readDetails->fileName<<\
                                    " from host : "<<GetHostName(i)<<"\n";
                                    SetClientReceivingFlag(i,false);
                                }

                            }
                            else
                            {
                                CommandDetails *cmdDetails = new CommandDetails;
                                memcpy(cmdDetails,buf,sizeof(CommandDetails));
                                if(cmdDetails->commandType == CONNECT)
                                {
                                    ClientList *recvClientList  = new ClientList;
                                    memcpy(recvClientList,buf+sizeof(CommandDetails),sizeof(ClientList));
                                    AddClientToList(recvClientList->IP,recvClientList->portNumber,i);
                                }
                                else if(cmdDetails->commandType == GET)
                                {
                                    FileNameDetails *fName = new FileNameDetails;
                                    memcpy(fName,buf+sizeof(CommandDetails),sizeof(FileNameDetails));
                                    cout<<"GET Command Received for file name : "<<fName->fileName<<"\n";
                                    _ExecutePUT(i,fName->fileName);
                                    delete fName;
                                }
                                else if(cmdDetails->commandType == PUT)
                                {

                                    long long int fileSize = cmdDetails->len;

                                    if(fileSize == -1)
                                    {
                                        cout<<"File open/not found error in other client, can't download it\n";
                                        continue;
                                    }

                                    if(!DiskSpaceLeft(fileSize))
                                    {
                                        cout<<"Error!!! No disk space left , can't write the file,ignore\n";
                                        continue;
                                    }
                             
                                    long long int bytesRead = nbytes-sizeof(CommandDetails)-sizeof(FileNameDetails);
                                    long long int bytesLeft = fileSize-bytesRead;

                                    FileNameDetails *fName = new FileNameDetails;
                                    memcpy(fName,buf+sizeof(CommandDetails),sizeof(FileNameDetails));
                                    char *_fileName = new char[strlen(fName->fileName)+12];

                                    char *dotPosition = strrchr(fName->fileName,'.');
                                    if(dotPosition != NULL)
                                    {
                                        strncpy(_fileName,fName->fileName,dotPosition-(fName->fileName));
                                        _fileName[dotPosition-(fName->fileName)] = '\0';
                                  
                                        strcat(_fileName,"_download");
                                        strcat(_fileName,(fName->fileName)+(dotPosition-(fName->fileName)));
                                    }
                                    else
                                    {
                                        strcpy(_fileName,fName->fileName);
                                        strcat(_fileName,"_download");
                                    }

                                    ofstream fileToWrite(_fileName, ios::out | ios::binary);
                                    if(!fileToWrite.is_open())
                                    {
                                        cout<<"File not found/Open Error in read, abort\n";
                                        continue;
                                    }

                                    char _buff[944];

                                    memcpy(_buff,buf+sizeof(CommandDetails)+sizeof(FileNameDetails),bytesRead);

                                    cout<<"Download Started for the file : "<<fName->fileName<<"\n";

                                    fileToWrite.write(_buff,bytesRead);

                                    if(bytesLeft > 0)
                                    {
                                        AddFileReadDetails(i,bytesRead,bytesLeft,_fileName);
                                        SetClientReceivingFlag(i,true);
                                    }
                                    else
                                    {
                                        cout<<"Download Completed for the file : "<<fName->fileName<<" in one go from the host : "\
                                        <<GetHostName(i)<<"\n";
                                    }

                                    fileToWrite.close();
                                    delete fName;
                                    delete _fileName;
                                }
                                else
                                {
                                    cout<<"Invalid Command Received\n";
                                }

                            }
                        }
                        memset(buf,0,BUFF_LEN);

                    }
                }
            }
	    //To upload some file to someother peer in a non-blocking way			
            if (FD_ISSET(i, &write_master))
            {
				if(!WriteComplete(i))
				{
					FileWriteDetails *fileToWrite = new FileWriteDetails;
					if(GetFileWriteDetails(i,fileToWrite) == -1)
					{
						cout<<"Get File Details Error\n";
						continue;
					}
					ifstream FileToSend(fileToWrite->fileName, ios::in | ios::binary);

					if(!FileToSend.is_open())
					{
						cout<<"File not found/Open Error in write, abort\n";
						continue;
					}

					char _buff[BUFF_LEN];
					FileToSend.seekg (fileToWrite->bytesWritten);
					FileToSend.read(_buff,BUFF_LEN);

					long long int bytesToWrite = fileToWrite->bytesToWrite;
					long long int bytesWritten = fileToWrite->bytesWritten;

					dout<<"Bytes written before send operation in PUT: "<<bytesWritten<<"\n";
					dout<<"Bytes to write before seed operation in PUT: "<<bytesToWrite<<"\n";

					long long int total = 0;
					long long int bytesleft = MIN(BUFF_LEN,bytesToWrite);
					long long int limit = bytesleft;
					long long int n;
					while(total < limit)
					{
						n = send(i,_buff+total, bytesleft, 0);
						if (n == -1)
						{
							perror("Send Error\n");
							break;
						}
						total += n;
						bytesleft -= n;
						bytesWritten += n;
					}

					dout<<"total : "<<total<<"\n";
					dout<<"bytesleft : "<<bytesleft<<"\n";

					bytesToWrite -= total;

					dout<<"bytesWritten : "<<bytesWritten<<"\n";
					dout<<"bytesToWrite : "<<bytesToWrite<<"\n";

					FileToSend.close();

					AddFileWriteDetails(i,bytesToWrite,bytesWritten,fileToWrite->fileName);

					if(bytesToWrite <= 0)
					{
						dout<<"Clearing Writefds Flag : \n";
						cout<<"Upload of the file : "<<fileToWrite->fileName<<" completed to the host : "\
						<<GetHostName(i)<<"\n";
						FD_CLR(i,&write_master);
					}
				}

			}
        }
    }
}


//To open listening port
void Client::OpenListeningPort(struct addrinfo hints)
{

    struct addrinfo *p,*ai;
    int rv = 0;
    int yes=1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char portTest[6] = {0,};

    HelperFunctions::PortNumToString(portNumber,portTest);

    if ((rv = getaddrinfo(NULL,portTest, &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectClient: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    if (listen(listener, 10) == -1)
    {
        perror("listen");
        exit(3);
    }

    freeaddrinfo(ai);
}

//To find port number using IP and fd
int Client::FindPortNumber(char *IP, int fd)
{
    ServerIPList *tempList = sList;
    dout<<" fd : "<<fd<<"\n";
    while(tempList != NULL)
    {
         dout<<"IP : "<<tempList->IP<<" fd : "<<tempList->fd<<"\n";
        if(tempList->fd == fd)
        {
            strcpy(IP,tempList->IP);
            return tempList->portNumber;
        }
        tempList = tempList->nextList;
    }
    dout<<"Port Number not found\n";
    return -1;
}

//Add the client details when a new connection is formed with other peer
void Client::AddClientToList(char *IP, int portNum, int fd)
{
    ClientList *tempList = new ClientList;

    int ipLen = strlen(IP);

    strncpy(tempList->IP,IP,ipLen);
    tempList->IP[ipLen] = '\0';

    tempList->portNumber = portNum;

    struct hostent *he;
    struct in_addr ipv4addr;

    inet_pton(AF_INET, IP, &ipv4addr);
    he = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    dout<<"Host name: "<<he->h_name<<"\n";
    int hNameLen = strlen(he->h_name);
    strncpy(tempList->hostName,he->h_name,hNameLen);
    tempList->hostName[hNameLen] = '\0';

    tempList->nextList = NULL;

    tempList->fd = fd;

    if(cList == NULL)
    {
        cList = tempList;
    }
    else if(serverFd == -1)
    {
        tempList->nextList = cList;
        cList = tempList;
    }
    else
    {
        ClientList *varList = cList;

        while(varList->nextList != NULL)
        {
            varList = varList->nextList;
        }

        varList->nextList = tempList;
    }

}

//Thsi API is used to set receiving flag to true or false
//so that we can know if any downloading is going on from 
//that fd
void Client::SetClientReceivingFlag(int fd,bool receivingFlag)
{
    if(cDetailsList == NULL)
    {
        dout<<"SetClientReceivingFlag1 recevingflag : "<<receivingFlag<<"\n";
        clientDetailsList *tList = new clientDetailsList;
        tList->fd = fd;
        tList->receivingFile = receivingFlag;
        tList->next = NULL;
        cDetailsList = tList;
    }
    else
    {
        dout<<"SetClientReceivingFlag2 recevingflag : "<<receivingFlag<<"\n";
        clientDetailsList *varList = cDetailsList;
        clientDetailsList *prevList = cDetailsList;
        while(varList != NULL && varList->fd != fd)
        {
            prevList = varList;
            varList = varList->next;
        }

        if(varList == NULL)
        {
            dout<<"SetClientReceivingFlag3 recevingflag : "<<receivingFlag<<"\n";
            clientDetailsList *tList = new clientDetailsList;
            tList->fd = fd;
            tList->receivingFile = receivingFlag;
            tList->next = NULL;
            prevList->next = tList;
        }
        else
        {
            dout<<"SetClientReceivingFlag4 recevingflag : "<<receivingFlag<<"\n";
           varList->receivingFile = receivingFlag;
        }
    }
}

//To check if the client is there or not in its connected list
bool Client::CheckClientDetailsFlag(int fd)
{
    clientDetailsList *tList = cDetailsList;

    if(tList == NULL)
    {
        dout<<"CheckClientDetailsFlag2 \n";
        return false;
    }
    else
    {
        while(tList != NULL)
        {
            dout<<"CheckClientDetailsFlag\n";

            if(tList->fd == fd)
            {
                if(tList->receivingFile == true)
                {
                    dout<<"CheckClientDetailsFlag3 \n";
                    return true;
                }
                else
                {
                    dout<<"CheckClientDetailsFlag4 \n";
                    return false;
                }
            }
            tList = tList->next;
        }
    }
    dout<<"CheckClientDetailsFlag5\n";
    return false;
}

void Client::UpdateServerList(ServerIPList *recvClientList)
{
    char *temp = (char*)recvClientList;
    int varSize = 0;

    ClearServerList();

    ServerIPList *firstList = new ServerIPList;
    memcpy(firstList,temp+varSize,sizeof(ServerIPList));

    sList = firstList;

    ServerIPList *prevList = firstList;

    varSize += sizeof(ServerIPList);

    while(prevList->nextList != NULL)
    {
        ServerIPList *tempList = new ServerIPList;
        memcpy(tempList,temp+varSize,sizeof(ServerIPList));

        prevList->nextList = tempList;
        prevList = tempList;
//        cout<<"IP : "<<tempList->IP<<"Port Num : "<<tempList->portNumber<<"\n";
        varSize += sizeof(ServerIPList);
    }
}

//To clear all the server list
void Client::ClearServerList()
{
    ServerIPList *tempList;

    while(sList != NULL)
    {
            tempList = sList;
            sList = sList->nextList;
            delete tempList;
    }
}

//To check if the IP the clients want to connect is valid or not
bool Client::ValidClientIP(char *destIP, int portNum)
{

    if((strcmp(destIP,hostIP) == 0) && (portNum == portNumber))
    {
        cout<<"Error!!! Trying to make self connection\n";
        return false;
    }

    ClientList *_clientList = cList;

    while(_clientList != NULL)
    {
        if(strcmp(_clientList->IP,destIP) == 0 && _clientList->portNumber == portNum)
        {
            cout<<"Error!!! Trying to make Duplicate connection\n";
            return false;
        }
        _clientList = _clientList->nextList;
    }


    ServerIPList *tempList = sList;

    while(tempList != NULL)
    {
        if(strcmp(tempList->IP,destIP) == 0 && (portNum == tempList->portNumber))
        {
            return true;
        }
        tempList = tempList->nextList;
    }

    cout<<"Error!!!, Client IP not found in Server IP List\n";
    return false;
}

//To get the host name
char* Client::GetHostName(int fd)
{

    ClientList *tempList = cList;

    for(int i = 1; tempList != NULL ; i++)
    {
        if(tempList->fd == fd)
        {
            return tempList->hostName;
        }
        tempList = tempList->nextList;
    }
    return (char*)"";
}

//To identify whether the input is Hostname or IP
bool Client::IdentifyIPorHostName(char *str)
{
    char *p = strtok(str, ".");
    int i = 0;
    char *tempStr;
    if(p == NULL)
    {
        return false;
    }
    while(p!= NULL && i < 4)
    {
        tempStr = p;
        if(!(HelperFunctions::CheckStringToIntValidity(tempStr)))
        {
            return false;
        }
        p = strtok(NULL,".");
        i++;
    }
    return true;
}

//To print server IP list
void Client::PrintServerIPList(ServerIPList *recvClientList)
{
    char *temp = (char*)recvClientList;

    cout<<"Server-IP List Update received from Server\n";

    int varSize = 0;
    while(1)
    {
        ServerIPList *tempList = new ServerIPList;
        memcpy(tempList,temp+varSize,sizeof(ServerIPList));
        cout<<"IP : "<<tempList->IP<<"\tPort Num : "<<tempList->portNumber<<"\n";
        if(tempList->nextList == NULL)
        {
            break;
        }
        varSize += sizeof(ServerIPList);
        delete tempList;
    }
}

Client *Client::s_client_instance = 0;
