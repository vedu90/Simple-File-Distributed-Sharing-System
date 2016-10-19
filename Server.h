#ifndef SERVER_H
#define SERVER_H

#include "SocketMain.h"

struct ServerIPList
{
    char IP[16];
    int portNumber;
    ServerIPList *nextList;
    int fd;
};

class Server:public SocketNetwork
{
    static Server *s_server_instance;
    int portNumber;
    ServerIPList *_serverIPList;
    ServerIPList *tailIPList;
    fd_set master;
    fd_set read_fds;
    Server():_serverIPList(NULL),tailIPList(NULL){}

    public:
    static Server *GetInstance()
    {
        if (!s_server_instance)
          s_server_instance = new Server;
        return s_server_instance;
    }
    void SetPort(int portNum);
    int GetPortNumber();
    void LaunchServer();
    void ExecuteRegister(char *serverIP, int portNum);
    void Manager();
    void addServerIPList(char *ip, int portNum, int fd);
    void deleteServerIPList(int fd);
    void deleteAllList();
    void CopyUpdateListToBuffer(char *buf,int *length);
    void *get_in_addr(struct sockaddr *sa);
    void PrintServerIPList();
    void ExecuteConnect(char *destIP, int portNum);
    void ExecuteList();
    void ExecuteTerminate(int connectionID);
    void ExecuteQuit();
    void ExecuteGET(int connectionID, char *fName);
    void ExecutePUT(int connectionID, char *fName);
};


#endif
