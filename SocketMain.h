/*Check .cpp file to see function comments*/
#ifndef SOCKET_MAIN_H
#define SOCKET_MAIN_H
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
//#include "client.h"
#define DISPLAY_MENU cout<<"\n1.HELP\n2.CREATOR\n3.DISPLAY\n4.REGISTER\n5.CONNECT\n6.LIST\n\
7.TERMINATE\n8.QUIT\n9.GET\n10.PUT\n"

//#define DEBUG 1
using namespace std;

#ifdef DEBUG
#define dout cout
#else
#define dout 0 && cout
#endif

class SocketNetwork
{
    public:
    char hostIP[32];
    char hostName[32];
    void LaunchConnection(SocketNetwork* obj);
    void ExecuteCommand(char *tokens[3], SocketNetwork* obj);
    virtual int GetPortNumber(){return 0;}
    void ExecuteHelp();
    void ExecuteCreator();
    void ExecuteDisplay(SocketNetwork* obj);
    virtual void ExecuteRegister(char *serverIP, int portNum){};
    virtual void ExecuteConnect(char *destIP, int portNum){};
    virtual void ExecuteList(){};
    virtual void Manager(){}
    virtual void ExecuteTerminate(int connectionID){}
    virtual void ExecuteQuit(){}
    virtual void ExecuteGET(int connectionID, char *fName){}
    virtual void ExecutePUT(int connectionID, char *fName){}
    void GetHostIPAndHostName();
};


class HelperFunctions
{
    public:
    static bool CheckStringToIntValidity(char *);
    static void ToLowerCase(char *myStr);
    static void TokenizeCommand(char*,char *tokens[3]);
    static const char* IntToString(int num);
    static void PortNumToString(int num,char *port);
};



#endif // SOCKET_MAIN_H
