
#include "TcpServer.h"

TcpServer::TcpServer()
    :   port(-1),
        epollWaitTimeout(500),
        bufferSize(4096),
        epollMaxSize(100),
        receiveThread(nullptr),
        receiveThreadCondition(false)
{
}

TcpServer::~TcpServer()
{
    releaseReceiveThread();
}

void TcpServer::onReceive(const int &clientSocketFd, 
                            char *data, 
                            const int &length)
{

}

void TcpServer::onDisconnect(const int &clientSocketFd)
{

}

void TcpServer::onConnectionExcetion(const int& clientSocketFd,
                                        const int& errorCode)
{

}

void TcpServer::setReceiveCallback(ReceiveCallback receiveCallback)
{
    this->receiveCallback = receiveCallback;
}

void TcpServer::setDisonnectCallback(DisconnectCallback disconnectCallback)
{
    this->disconnectCallback = disconnectCallback;
}

void TcpServer::setConnectionExceptionCallback(ConnectionExceptionCallback 
                                                connectionExceptionCallback)
{
    this->connectionExceptionCallback = connectionExceptionCallback;
}

void TcpServer::setBufferSize(const unsigned int &size)
{
    this->bufferSize = size;
}

void TcpServer::setEpollMaxSize(const unsigned int &size)
{
    this->epollMaxSize = size;
}

void TcpServer::config(const int& port,
                        int epollWaitTimeout)
{
    this->port = port;
    this->epollWaitTimeout = epollWaitTimeout;
}

bool TcpServer::listen()
{
    return listen(this->port);
}

bool TcpServer::listen(const int &port,
                        int epollWaitTimeout)
{
    this->port = port;
    this->epollWaitTimeout = epollWaitTimeout;
    receiveThread = new std::thread([&]() {
        /* server socket & address */
        int serverSocketFd = -1;
        struct sockaddr_in serverSocketAddr;
        bzero(&serverSocketAddr,sizeof(serverSocketAddr));

        /* create socket & bind serverSocketFd  */
        serverSocketFd = socket(AF_INET,SOCK_STREAM,0);
        serverSocketAddr.sin_family = AF_INET;
        serverSocketAddr.sin_port = htons(this->port);
        serverSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if(-1 == bind(serverSocketFd,
                        (struct sockaddr*)&serverSocketAddr,
                        sizeof(serverSocketAddr)))
        {
            printf("[TcpServer::listen(const int& port)]: failed by bind.\n");
            listenConditionVariable.notify_one();
            return;
        }

        /**
         * listen by ::listen (int __fd, int __n)
         * params:
         *      __fd means socket file description.
         *      __n means the length for __fd queue.
         */
        const int LISTEN_QUEUE_LENGTH = 10;
        if(-1 == ::listen(serverSocketFd,LISTEN_QUEUE_LENGTH))
        {
            printf("[TcpServer::listen(const int& port)]: failed by listen,"
                    "retry with another port\n");
            listenConditionVariable.notify_one();
            return;
        }

        /* event that to be registered into kernel */
        struct epoll_event registerEvent;
        /* event that the kernel had listened */
        struct epoll_event* listenedEvents = new epoll_event[epollMaxSize];

        /**
         * create epoll by ::epoll_create (int __size)
         * since Linux 2.6.8,the __size argument is ignored,
         * but must be greater than zero.
         */
        int epollFd = epoll_create(1);
        if(-1 == epollFd)
        {
            printf("[TcpServer::listen(const int& port)]: failed by epoll_create.\n");
            listenConditionVariable.notify_one();
            return;
        }

        /* register server serverSocketFd to epoll events */
        registerEvent.data.fd = serverSocketFd;
        registerEvent.events = EPOLLIN;
        int registerState = epoll_ctl(epollFd,EPOLL_CTL_ADD,serverSocketFd,&registerEvent);
        if(-1 == registerState)
        {
            printf("[TcpServer::listen(const int& port)]: failed by epoll_ctl.\n");
            listenConditionVariable.notify_one();
            return;
        }

        /* register other events & receive data from client socket */
        char* buffer = new char[bufferSize];
        memset(buffer,0,bufferSize*sizeof(char));
        receiveThreadCondition = true;
        listenConditionVariable.notify_one();
        printf("[TcpServer::listen(const int& port)]: tcp server listening at port %d.\n",port);
        
        /* local variable for loop  */
        int currentListenSize = 0;
        int listenedCount = -1;
        int i = 0;
        int receiveLength = -1;
        while(receiveThreadCondition)
        {   
            listenedCount = epoll_wait(epollFd,listenedEvents,epollMaxSize,this->epollWaitTimeout);
            if(-1 == listenedCount)
            {
                printf("[TcpServer::listen(const int& port)]: wait epoll event failed by unkown reason,"
                        "wait for other epoll events.\n");
                continue;
            }

            /* execute for listened events */
            for(i=0;i<listenedCount;i++)
            {   
                /* new client socket request */
                if(serverSocketFd == listenedEvents[i].data.fd &&
                    (EPOLLIN == listenedEvents[i].events & EPOLLIN))
                {
                    struct sockaddr_in clientAddress;
                    int clientLength = sizeof(clientAddress);

                    /* accept client socket file description */
                    int clientSocketFd = accept(serverSocketFd,
                                                (struct sockaddr*)&clientAddress,
                                                (socklen_t*)&clientLength);
                    if(-1 == clientSocketFd)
                    {
                        printf("[TcpServer::listen(const int& port)]: accept client socket failed by unkown reason,"
                                "execute other epoll events.\n");
                        continue;
                    }
                    /* register event */
                    registerEvent.data.fd = clientSocketFd;
                    registerEvent.events = EPOLLIN;
                    int registerState = epoll_ctl(epollFd,EPOLL_CTL_ADD,clientSocketFd,&registerEvent);
                    if(-1 == registerState)
                    {
                        printf("[TcpServer::listen(const int& port)]: register client socket failed by unkown reason,"
                                "wexecute other epoll events.\n");
                        continue;
                    }
                    else
                    {
                        printf("[TcpServer::listen(const int& port)]: new client %d connected and "
                                "had register to epoll events.\n",clientSocketFd);
                    }
                }
                else
                {
                    receiveLength = recv(listenedEvents[i].data.fd,
                                                buffer,
                                                bufferSize,
                                                0);
                    /* receive exception */
                    if (receiveLength <= 0)
                    {
                        if(0 != receiveLength)
                        {
                            /* client connection exception: reset,timeout... */
                            switch (errno)
                            {
                            case ECONNRESET:
                                printf("[TcpServer::listen(const int& port)]: client %d connection reset.\n",
                                        listenedEvents[i].data.fd);
                                break;
                            case ETIMEDOUT:
                                printf("[TcpServer::listen(const int& port)]: client %d connection timeout.\n",
                                        listenedEvents[i].data.fd);
                                break;
                            default:
                                printf("[TcpServer::listen(const int& port)]: receive failed from client by unkown reason,"
                                        "error code is %d.\n",errno);
                                break;
                            }
                            /* connection exception callback */
                            onConnectionExcetion(listenedEvents[i].data.fd,errno);
                            if(nullptr != connectionExceptionCallback)
                            {
                                connectionExceptionCallback(listenedEvents[i].data.fd,errno);
                            }
                        }
                        else
                        {
                            /* client disconnect */
                            printf("[TcpServer::listen(const int& port)]: client %d had disconnected.\n",
                                    listenedEvents[i].data.fd);
                            /* disconnect callback */
                            onDisconnect(listenedEvents[i].data.fd);
                            if(nullptr != disconnectCallback)
                            {
                                disconnectCallback(listenedEvents[i].data.fd);
                            }
                        }
                        
                        ::close(listenedEvents[i].data.fd);
                        epoll_ctl(epollFd, EPOLL_CTL_DEL,listenedEvents[i].data.fd, &registerEvent);
                        continue;
                    }
                    else /* receive success */
                    {
                        /* receive callback */
                        onReceive(listenedEvents[i].data.fd,buffer,receiveLength);
                        if(nullptr != receiveCallback)
                        {
                            receiveCallback(listenedEvents[i].data.fd,buffer,receiveLength);
                        }
                        memset(buffer,0,bufferSize);
                    }
                }
            }
        }

        /* finally release for thread */
        printf("[TcpServer::listen(const int& port)]: listen thread finished, clearing memory now.\n.");
        ::close(serverSocketFd);
        serverSocketFd = -1;
        ::close(epollFd);
        epollFd = -1;
        delete[] listenedEvents;
        listenedEvents = nullptr;
        delete[] buffer;
        buffer = nullptr;
        printf("[TcpServer::listen(const int& port)]: memory had been cleaned.\n.");
    });

    std::unique_lock<std::mutex> lock(listenMutex);
    listenConditionVariable.wait(lock);
    return receiveThreadCondition;
}

void TcpServer::close()
{
    releaseReceiveThread();
}

int TcpServer::send(const int &clientSocketFd,
                    char *data,
                    const int &length)
{
    int sendLength = 0;
    sendLength = ::send(clientSocketFd, data, length, 0);
    return sendLength;
}

void TcpServer::releaseReceiveThread()
{
    receiveThreadCondition = false;
    /** 
     * pseudo client socket for indefinite epoll_wait 
     * that: epollWaitTimeout equal to -1
     **/
    if(-1 == epollWaitTimeout)
    {
        int pseudoClientFd = -1;
        sockaddr_in serverAddr;
        pseudoClientFd = socket(AF_INET,SOCK_STREAM,0);
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serverAddr.sin_port = htons(this->port);
        if(0 == connect(pseudoClientFd,(struct sockaddr*)&serverAddr,sizeof(serverAddr)))
        {
            printf("[TcpServer::releaseReceiveThread()]: pseudo client socket for unblock epoll_wait,"
                    "it is normal that will triggle an new client socket connected epoll event.\n");
            ::close(pseudoClientFd);
        }
    }
    receiveThread->join();
    delete receiveThread;
    receiveThread = nullptr;
}