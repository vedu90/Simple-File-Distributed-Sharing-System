/*
This is the main file where the process execution is started.
*/

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/unistd.h>
#include <iostream>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "Client.h"
#include "Server.h"

//Convert integer to string
const char* HelperFunctions::IntToString(int num)
{
    std::stringstream ss;
    ss << num;
    std::string str = ss.str();
    return str.c_str();
}

//Convert port number to string
void HelperFunctions::PortNumToString(int num,char *port)
{
    int temp = num;
    int len = 0;
    while(temp != 0)
    {
        temp = temp/10;
        len++;
    }

    int k = len-1;
    temp = num;
    while(temp != 0)
    {
        int rem = temp%10;
        temp = temp/10;
        port[k] = rem+48;
        k--;
    }
    port[len] = '\0';
}

//Check if the given port number is valid or not
bool HelperFunctions::CheckStringToIntValidity(char *portNumber)
{
    for (int i = 0; i < strlen (portNumber); i++)
    {
        if (!std::isdigit(portNumber[i]))
        {
            return false;
        }
    }
    return true;
}

//Converts to lower case
void HelperFunctions::ToLowerCase(char *myStr)
{
    for (int i = 0; i < strlen(myStr); i++)
    {
        if (std::isalpha(myStr[i]))
        {
            myStr[i] = std::tolower(myStr[i]);
        }
    }
}

//Tokenizes the input commands
void HelperFunctions::TokenizeCommand(char *myStr,char *tokens[3])
{
    char *p = strtok(myStr, " ");
    int i = 0;
    while(p!= NULL && i < 3)
    {
        tokens[i] = p;
        p = strtok(NULL," ");
        i++;
    }
}

//To launch the connection for server or client
void SocketNetwork::LaunchConnection(SocketNetwork* obj)
{
    GetHostIPAndHostName();
    obj->Manager();
}

//Upon entering the command to be executed, this API is called where depending on the Command Type and 
//process type i.e Client or Server, its corresponding API is called
void SocketNetwork::ExecuteCommand(char *commandType[3],SocketNetwork* obj)
{
    if(strcmp(commandType[0],"help") == 0)
    {
        cout<<"Help Command Executed\n";
        ExecuteHelp();
    }
    else if(strcmp(commandType[0],"creator") == 0)
    {
        cout<<"Creator Command Executed\n";
        ExecuteCreator();
    }
    else if(strcmp(commandType[0],"display") == 0)
    {
        cout<<"Display Command Executed\n";
        ExecuteDisplay(obj);
    }
    else if(strcmp(commandType[0],"register") == 0)
    {
        cout<<"Register Command Executed\n";
        if(commandType[1] != NULL && commandType[2] != NULL)
        {
            int portNum = atoi(commandType[2]);
            obj->ExecuteRegister(commandType[1],portNum);
        }
        else
        {
            cout<<"Error!!! Total 3 arguments expected\n";
        }

    }
    else if(strcmp(commandType[0],"connect") == 0)
    {
        cout<<"Connect Command Executed\n";
        if(commandType[1] != NULL && commandType[2] != NULL)
        {
            int portNum = atoi(commandType[2]);
            obj->ExecuteConnect(commandType[1],portNum);
        }
        else
        {
            cout<<"Error!!! Total 3 arguments expected\n";
        }

    }
    else if(strcmp(commandType[0],"list") == 0)
    {
        cout<<"List Command executed\n";
        obj->ExecuteList();
    }
    else if(strcmp(commandType[0],"terminate") == 0)
    {
        cout<<"Terminate Command executed\n";
        if(commandType[1] != NULL)
        {
            int connectionID = atoi(commandType[1]);
            obj->ExecuteTerminate(connectionID);
        }
        else
        {
            cout<<"Error!!! Total 2 arguments expected\n";
        }
    }
    else if(strcmp(commandType[0],"quit") == 0)
    {
        cout<<"Quit Command executed\n";
        obj->ExecuteQuit();
    }
    else if(strcmp(commandType[0],"get") == 0)
    {
        cout<<"Get Command executed\n";
        if(commandType[1] != NULL && commandType[2] != NULL)
        {
            int connectionID = atoi(commandType[1]);
            obj->ExecuteGET(connectionID,commandType[2]);
        }
        else
        {
            cout<<"Error!!! Total 3 arguments expected\n";
        }
    }
    else if(strcmp(commandType[0],"put") == 0)
    {
        cout<<"Put Command executed\n";
        if(commandType[1] != NULL && commandType[2] != NULL)
        {
            int connectionID = atoi(commandType[1]);
            obj->ExecutePUT(connectionID,commandType[2]);
        }
        else
        {
            cout<<"Error!!! Total 3 arguments expected\n";
        }
    }
    else
    {
        cout<<"Invalid Command\n";
    }
}


//Help command is executed below where all the supported commands with their description is shown on the UI
//This API is common for Server and Client
void SocketNetwork::ExecuteHelp()
{
    cout<<"\n***Available User Commands are shown below with desciption***\n\n";

    cout<<"1.HELP\n  ---Displays information about the available user command options.\n\n";

    cout<<"2.CREATOR\n  ---Displays student's full name, UBIT name and UB email address.\n\n";

    cout<<"3.DISPLAY\n  ---Displays the IP address of this process, and the port on which this process is listening for\
 incoming connections\n\n";

    cout<<"4.REGISTER <server IP> <port no>\n  ---This command is used by the client to register itself with the \
server and to get the IP and listening port numbers of all other peers currently registered with the \
server.The REGISTER command takes 2 arguments. The first \
argument is the IP address of the server and the second argument is the listening port of the server.\n\n";

    cout<<"5.CONNECT <destination> <port no>\n  ---This command is used to establish a connection between two \
registered clients. The command establishes a new TCP connection to the specified <destination> at the \
specified <port no>. The <destination> can either be an IP address or a hostname.\n\n";

    cout<<"6.LIST\n  ---Displays a numbered list of all the connections this process is part of. This numbered list will \
include connections initiated by this process and connections initiated by other processes.\n\n";

    cout<<"7.TERMINATE <connection id>\n  ---This command will terminate the connection listed under the \
specified number when LIST is used to display all connections.\n\n";

    cout<<"8.QUIT\n  ---Close all connections and terminate this process.\n\n";

    cout<<"9.GET <connection id> <file>\n  ---This command will download a file from one host specified in the \
command.\n\n";

    cout<<"10.PUT <connection id> <file name>\n  ---This command will upload the file(in the filename argument) to the host \
    on the connection (whose connection id is equal to connection id in the argument).\n\n";

}


//Creator command is executed below.
//This API is common for Server and Client
void SocketNetwork::ExecuteCreator()
{
    cout<<"\nStudent's Full Name : XXXXXX\n";
    cout<<" University ID : XXXXXXX\n";
    cout<<"email address : XXXXXXX\n";
}

//Display command is executed below where hostname, IP address and Port Number are displayed
//This API is common for Server and Client
void SocketNetwork::ExecuteDisplay(SocketNetwork* obj)
{
    cout<<"Host Name : "<<hostName<<"\n";
    cout<<"IP address : "<<hostIP<<"\n";
    cout<<"Port Number : "<<obj->GetPortNumber()<<"\n";
}

// API to get the Hostname and Host IP
void SocketNetwork::GetHostIPAndHostName()
{

    if(!gethostname(hostName,32))
    {
        struct hostent *temp;
        if((temp = gethostbyname(hostName)) != NULL)
        {
            struct in_addr **addr_list;
            addr_list = (struct in_addr **)temp->h_addr_list;
            strcpy(hostIP,inet_ntoa(*addr_list[0]));
            dout<<"IP address : "<<hostIP<<"\n";
            dout<<"Host Name : "<<hostName<<"\n";
        }
        else
        {
            perror("gethostbyname");
        }

    }
    else
    {
        perror("gethostname");
    }
}

//Takes the 2 input commands from the command line
int main(int argc , char *argv[])
{
    Server *myServer = NULL;
    Client *myClient = NULL;
    SocketNetwork *pObject = NULL;
    if (argc < 3 || argc > 3)
    {
        cout<<"Error!!! expecting 2 command line arguments\n";
        return -1;
    }

    //Checking if you want to start as server
    if(strlen(argv[1]) == 1 && strncmp(argv[1],"s",1) == 0)
    {
        myServer = Server::GetInstance();
        pObject = myServer;
        //Checking if the port number is valid or not
        if(HelperFunctions::CheckStringToIntValidity(argv[2]) == true)
        {
            int portNum = atoi(argv[2]);
            if(portNum < 65536)
            {
                cout<<"Launch Server Instance with port number : "<<portNum;
                //Launching Server
                myServer->SetPort(portNum);
                DISPLAY_MENU;
                pObject->LaunchConnection(pObject);
            }
            else
            {
                cout<<"Invalid port Number\n";
            }
        }
        else
        {
              cout<<"Invalid port Number\n";
        }

    }
    //Checking if you want to start as client
    else if(strlen(argv[1]) == 1 && strncmp(argv[1],"c",1) == 0)
    {
        dout<<"Client\n";
        myClient = Client::GetInstance();
        pObject = myClient;
         //Checking if the port number is valid or not
        if(HelperFunctions::CheckStringToIntValidity(argv[2]) == true)
        {
            int portNum = atoi(argv[2]);
            if(portNum < 65536)
            {
                cout<<"Launch Client Instance with port number : "<<portNum;
                //Launching Client
                myClient->SetPort(portNum);
                DISPLAY_MENU;
                pObject->LaunchConnection(pObject);
            }
            else
            {
                cout<<"Invalid port Number\n";
            }
        }
        else
        {
              cout<<"Invalid port Number\n";

        }

    }
    else
    {
        cout<<"Error!!! Expected \'s\' or \'c\' as first argument\n";
    }
    return 0;
}
