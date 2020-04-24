/**
 * class:   TcpServer
 * author:  Ouyang Wei
 * contact: ouyang_wei_112@126.com
 * description:
 *      The TcpServer is a tcp server library implement by epoll.
 *      Please view main.cpp to get the demo codes that how to use it.
 *      It supports both C and C++ callback with std::function object, 
 *      and it provides support for polymorphism with member virtual  
 *      functions you had seen.
 */

#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <signal.h>

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

using ReceiveCallback = std::function<void (const int&,
                                        char*,
                                        const int&)>;
using DisconnectCallback = std::function<void (const int&)>;
using ConnectionExceptionCallback = std::function<void (const int&,
                                                    const int&)>;

class TcpServer
{
public:
    TcpServer();
    virtual ~TcpServer();

    virtual void onReceive(const int& clientSocketFd,
                            char* data,
                            const int& length);
    virtual void onDisconnect(const int& clientSocketFd);
    /**
     * errorCode:
     *      104 ->  connection reset
     *      110 ->  connection timeout
     * more error codes are descripted in:
     *      http://man7.org/linux/man-pages/man3/errno.3.html
     */
    virtual void onConnectionExcetion(const int& clientSocketFd,
                                        const int& errorCode);

    void setReceiveCallback(ReceiveCallback receiveCallback);
    void setDisonnectCallback(DisconnectCallback disconnectCallback);
    void setConnectionExceptionCallback(ConnectionExceptionCallback 
                                        connectionExceptionCallback);

    void setBufferSize(const unsigned int& size);
    void setEpollMaxSize(const unsigned int& size);

    /**
     * epollWaitTimeout:
     *      -1: indefinite (deprecated if unnecessary)
     */
    void config(const int& port,
                int epollWaitTimeout = 500 /* unit: ms */);
    bool listen();
    bool listen(const int& port,
                int epollWaitTimeout = 500 /* unit: ms */);
    void close();

    int send(const int& clientSocketFd,
            char* data,
            const int& length);

private:
    void releaseReceiveThread();

private:
    int port;
    int epollWaitTimeout;
    unsigned int bufferSize;
    unsigned int epollMaxSize;
    std::thread* receiveThread;
    bool receiveThreadCondition;
    std::mutex listenMutex;
    std::condition_variable listenConditionVariable;
    ReceiveCallback receiveCallback;
    DisconnectCallback disconnectCallback;
    ConnectionExceptionCallback connectionExceptionCallback;
};

#endif