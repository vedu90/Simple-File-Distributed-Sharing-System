/*Check .cpp file to see function comments*/
#ifndef CLIENT_H
#define CLIENT_H
#include "SocketMain.h"
#include "Server.h"
#include<netdb.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include<string.h>

struct ClientList
{
    char hostName[40];
    char IP[16];
    int portNumber;
    int fd;
    ClientList *nextList;
};

struct clientDetailsList
{
    int fd;
    bool receivingFile;
    clientDetailsList *next;
};

struct CommandDetails
{
    int commandType; //5:Connect,9:GET,10: PUT
    long long int len;
};

struct FileNameDetails
{
    char fileName[64];
};

struct FileReadDetails
{
    int fd;
    long long int bytesLeft;
    long long int bytesRead;
    char fileName[64];
    FileReadDetails *next;
};

struct FileWriteDetails
{
	int fd;
	long long int bytesWritten;
	long long int bytesToWrite;
	char fileName[64];
	FileWriteDetails *next;
};


class Client:public SocketNetwork
{
    static Client *s_client_instance;
    int serverFd;
    int portNumber;
    fd_set read_master;
    fd_set write_master;
    fd_set read_fds;
    fd_set write_fds;
    int fdmax;
    int peerConnections;
    bool isServerConnected;
    int listener;
    ServerIPList *sList;
    ClientList *cList;
    clientDetailsList *cDetailsList;
    FileWriteDetails *firstWriteFile;
    FileReadDetails *firstReadFile;
    struct addrinfo hints;
    Client():sList(NULL),cList(NULL),serverFd(-1),portNumber(0),fdmax(0),cDetailsList(NULL),\
    firstReadFile(NULL),firstWriteFile(NULL),peerConnections(0),isServerConnected(false),listener(0)
    {
        memset(&hints,0,sizeof(struct addrinfo));
    }

    public:
    static Client *GetInstance()
    {
        if (!s_client_instance)
          s_client_instance = new Client;
        return s_client_instance;
    }
    void SetPort(int portNum);
    int GetPortNumber();
    void LaunchClient();
    void ExecuteRegister(char *serverIP, int portNum);
    void Manager();
    void PrintServerIPList(ServerIPList *recvClientList);
    void ExecuteConnect(char *destIP, int portNum);
    void UpdateServerList(ServerIPList *recvClientList);
    void ClearServerList();

    bool ValidClientIP(char *destIP, int portNum);
    void AddClientToList(char *IP, int portNum,int fd);
    void ExecuteList();
    void ExecuteTerminate(int connectionID);
    int FindPortNumber(char *IP, int fd);
    void ExecuteQuit();
    int GetFileWriteDetails(int fd,struct FileWriteDetails *fDetails);
    void AddFileWriteDetails(int fd,long long int bToWrite,long long int bWritten,char *fName);
    bool WriteComplete(int fd);
    int GetFileReadDetails(int fd,struct FileReadDetails *fDetails);
    void AddFileReadDetails(int fd,long long int bytesRead,long long int bytesLeft,char *fileName);
    void ExecuteGET(int connectionID, char *fName);
    void ExecutePUT(int connectionID, char *fName);
    void _ExecutePUT(int sockfd, char *filePath);
    char* GetHostName(int fd);

    void SetClientReceivingFlag(int fd,bool receivingFlag);
    bool CheckClientDetailsFlag(int fd);

    void OpenListeningPort(struct addrinfo hints);
    bool IdentifyIPorHostName(char *str);
    bool DiskSpaceLeft(long long int fileSize);
};


#endif
