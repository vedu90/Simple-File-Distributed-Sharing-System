#include<signal.h>
#include "Server.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<errno.h>
#include<sys/wait.h>
#include "Defines.h"

void Server::ExecuteRegister(char *serverIP, int portNum)
{
    cout<<"Register command not supported for Server\n";
}

void Server::ExecuteConnect(char *destIP, int portNum)
{
    cout<<"Connect command not supported for Server\n";
}

void Server::ExecuteList()
{
    cout<<"List command not supported for Server\n";
}

void Server::ExecuteTerminate(int connectionID)
{
    cout<<"Terminate command not supported for Server\n";
}

void Server::ExecuteQuit()
{
    ServerIPList *tempList = _serverIPList;

    cout<<"Quit command received, the server should quit\n";

    ServerIPList *prevList;
    while(tempList != NULL)
    {
        dout<<"FD : "<<tempList->fd<<"\n";
        FD_CLR(tempList->fd, &master);
        FD_CLR(tempList->fd, &read_fds);
        close(tempList->fd);
        prevList = tempList;
        tempList = tempList->nextList;

        delete prevList;
    }
    cout<<"Server Quitting, bye-bye\n";
    exit(0);


}

void Server::ExecuteGET(int connectionID, char *fName)
{
    cout<<"GET command not supported for Server\n";
}

void Server::ExecutePUT(int connectionID, char *fName)
{
    cout<<"PUT command not supported for Server\n";
}

#if 1

void *Server::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
}


void Server::Manager()
{
    dout<<"Debug "<<__LINE__<<"\n";
    int fdmax;
    int listener;
    int newfd;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;
    char buf[BUFF_LEN];
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    int yes=1;
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    dout<<"Debug "<<__LINE__<<"\n";
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char portTest[6] = {0,};

    HelperFunctions::PortNumToString(portNumber,portTest);

    if ((rv = getaddrinfo(NULL, portTest, &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    for(p = ai; p != NULL; p = p->ai_next)
    {
        dout<<"Debug "<<__LINE__<<"\n";
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }
        if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }
        break;
    }
    dout<<"Debug "<<__LINE__<<"\n";
    if (p == NULL)
    {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai);

    dout<<"Debug "<<__LINE__<<"\n";
    //Adding STDIN

    if (listen(listener, 10) == -1)
    {
        perror("listen");
        exit(3);
    }
    dout<<"Debug "<<__LINE__<<"\n";
    FD_SET(0, &master);
    FD_SET(listener, &master);
    fdmax = listener;
    dout<<"Listener "<<listener<<"\n";

    for(;;)
    {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }
        dout<<"Debug "<<__LINE__<<"\n";
        for(i = 0; i <= fdmax; i++)
        {
            dout<<"Debug "<<__LINE__<<"\n";
            if (FD_ISSET(i, &read_fds))
            {
                if(i == 0)
                {
                    dout<<"STDIN Server Manager\n";
                    std::string strCommand;
                    getline(cin,strCommand);

                    char *commandTokens[3] = {NULL,NULL,NULL};
                    HelperFunctions::TokenizeCommand((char *)strCommand.c_str(),commandTokens);
                    HelperFunctions::ToLowerCase(commandTokens[0]);
                    dout<<commandTokens[0]<<endl;
                    ExecuteCommand(commandTokens,this);
                }
                else if (i == listener)
                {
                    dout<<"Debug "<<__LINE__<<"\n";
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
                        FD_SET(newfd, &master);
                        if (newfd > fdmax)
                        {
                            fdmax = newfd;
                        }
                        printf("Recieved new client connection from %s on socket %d\n", \
                               inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr),\
                                         remoteIP, INET_ADDRSTRLEN),newfd);


                    }

                }
                else
                {
                    if ((nbytes = recv(i,(void*)buf, sizeof(buf), 0)) <= 0)
                    {
                        dout<<"Debug "<<__LINE__<<"\n";
                        if(nbytes == 0)
                        {
                            printf("Client on socket %d hung up\n", i);
                            deleteServerIPList(i);
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);

                        int bufLen = 0;
                        memset(buf,0,BUFF_LEN);
                        CopyUpdateListToBuffer(buf,&bufLen);
                        int k = 0;
                        for(k = 1; k <= fdmax; k++)
                        {
                            if (FD_ISSET(k, &master))
                            {
                                if (k != listener)
                                {
                                    int total = 0;

                                    int bytesleft = bufLen;
                                    int n;
                                    while(total < bufLen)
                                    {
                                        n = send(k, buf+total, bytesleft, 0);
                                        if (n == -1)
                                        {
                                            perror("send");
                                        }
                                        total += n;
                                        bytesleft -= n;
                                    }
                                }
                            }
                        }


                    }
                    else
                    {
                        dout<<"Debug "<<__LINE__<<"\n";
                        ServerIPList *recvClientList = (ServerIPList*)buf;
                        int bufLen = 0;
                        addServerIPList(recvClientList->IP,recvClientList->portNumber,i);
                        PrintServerIPList();
                        memset(buf,0,BUFF_LEN);
                        CopyUpdateListToBuffer(buf,&bufLen);
                        for(j = 1; j <= fdmax; j++)
                        {
                            if (FD_ISSET(j, &master))
                            {
                                if (j != listener)
                                {
                                    int total = 0;

                                    int bytesleft = bufLen;
                                    int n;
                                    while(total < bufLen)
                                    {
                                  //      cout<<"Checking bytesleft : "<<bytesleft<<"\n";
                                        n = send(j, buf+total, bytesleft, 0);
                                        if (n == -1)
                                        {
                                            perror("send");
                                        }
                                        total += n;
                                        bytesleft -= n;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


#endif
void Server::PrintServerIPList()
{
    ServerIPList *tempList = _serverIPList;

    while(tempList != NULL)
    {
        cout<<"IP : "<<tempList->IP<<"\tPort Num : "<<tempList->portNumber<<"\n";
        tempList = tempList->nextList;
    }
}


void Server::addServerIPList(char *ip, int portNum,int fd)
{
    int ipLen = strlen(ip);
    ServerIPList *newServerIP = new(std::nothrow) ServerIPList;

    if(newServerIP == NULL)
    {
        cout<<"Memory Allocation Error\n";
        return;
    }

    strncpy(newServerIP->IP,ip,ipLen);
    newServerIP->IP[ipLen] = '\0';

    newServerIP->portNumber = portNum;

    newServerIP->fd = fd;

    newServerIP->nextList = NULL;

    if(_serverIPList == NULL)
    {
        _serverIPList = newServerIP;
        tailIPList = _serverIPList;
    }
    else
    {
        tailIPList->nextList = newServerIP;
        tailIPList = newServerIP;
    }
}

void Server::deleteServerIPList(int fd)
{
    if(_serverIPList == NULL)
    {
        cout<<"Server IP List, nothing to delete\n";
        return;
    }

    ServerIPList *delList = _serverIPList;
    ServerIPList *prevList = NULL;
    if(fd == _serverIPList->fd)
    {
        _serverIPList = _serverIPList->nextList;
    }
    else
    {
        prevList = delList;
        delList = delList->nextList;
        while(delList != NULL && fd != delList->fd)
        {
            prevList = delList;
            delList = delList->nextList;
        }

        if(delList == NULL)
        {
            cout<<"Server IP List to be deleted not found \n";
            return;
        }
        if(delList == tailIPList)
        {
            tailIPList = prevList;
        }
        prevList->nextList = delList->nextList;
        delete delList;
    }
}

void Server::deleteAllList()
{
    if(_serverIPList == NULL)
    {
        cout<<"Server IP List, nothing to delete\n";
        return;
    }

    ServerIPList *tempList = _serverIPList;
    while(_serverIPList != NULL)
    {
        tempList = _serverIPList;
        _serverIPList = _serverIPList->nextList;
        delete tempList;
    }
}

void Server::CopyUpdateListToBuffer(char *buf, int *length)
{
    ServerIPList *tempList = _serverIPList;
    int varSize = 0;
    while(tempList != NULL)
    {
        memcpy((void *)(buf+varSize),tempList,sizeof(ServerIPList));
        varSize += (sizeof(ServerIPList));
    //    cout<<"varSize : "<<varSize<<"\n";
        tempList = tempList->nextList;
    }

    char *temp = (char*)buf;

    varSize = 0;
    while(1)
    {
      //  cout<<"Debug man\n";
        ServerIPList *tempList = new ServerIPList;
        memcpy(tempList,temp+varSize,sizeof(ServerIPList));
      //  cout<<"Debug man\n";
   //     cout<<"IP : "<<tempList->IP<<"Port Num : "<<tempList->portNumber<<" fd : "<<tempList->fd<<"\n";

        varSize += sizeof(ServerIPList);
        if(tempList->nextList == NULL)
        {
            break;
        }

        delete tempList;
    }

    *length = varSize+1;
}

void Server::SetPort(int portNum)
{
    portNumber = portNum;
}
int Server::GetPortNumber()
{
    return portNumber;
}


Server *Server::s_server_instance = 0;
