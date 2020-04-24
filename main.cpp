
#include <iostream>

#include "TcpServer.h"

class TcpServerImpl: public TcpServer
{
public:
    void onReceive(const int& clientSocketFd,char* data,const int& length)
    {
        if(length > 0)
        {
            printf("tcp server1 receive: %s , length: %d\n",data,length);
        }
        std::string response("receive");
        this->send(clientSocketFd,(char*)response.c_str(),response.size());
    }
    void onDisconnect(const int& clientSocketFd)
    {
        printf("tcp server1 client %d disconnected.\n",clientSocketFd);
    }
    void onConnectionExcetion(const int& clientSocketFd,
                                const int& errorCode)
    {
        printf("tcp server1 client %d connection exception by code %d.\n",
                clientSocketFd,
                errorCode);
    }
};

int main(int argc,char** argv)
{
    printf("==============tcp server=============\n");
    
    /* 1 */
    TcpServerImpl server1;
    server1.setEpollMaxSize(10000);
    if(server1.listen(12345))
    {
        printf("tcp server1 listening...\n");
    }
    else
    {
        printf("tcp server1 listen failed...\n");
    }

    /* 2 */
    TcpServer server2;
    server2.setReceiveCallback([&](const int& clientSocketFd,char* data,const int& length){
        if(length > 0)
        {
            printf("tcp server2  ReceiveCallback: %s , length: %d\n",data,length);
        }
        std::string response("setReceiveCallback");
        server2.send(clientSocketFd,(char*)response.c_str(),response.size());
    });
    server2.setDisonnectCallback([&](const int& clientSocketFd){
        printf("tcp server2 client %d disconnected.\n",clientSocketFd);
    });
    
    if(server2.listen(54321,-1))
    {
        printf("tcp server2 listening...\n");
    }
    else
    {
        printf("tcp server2 listen failed...\n");
    }

    getchar();

    return 0;
}