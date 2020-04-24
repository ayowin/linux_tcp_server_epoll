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

int main(int argc,char** argv)
{
	printf("=========epoll clients test================\n");
	
	const long clientLength = 100000;

	int clients[clientLength];
	memset(clients,-1,clientLength*sizeof(int));
	
	
	for(long i=0;i<clientLength;i++)
	{
        sockaddr_in serverAddr;
        clients[i] = socket(AF_INET,SOCK_STREAM,0);
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serverAddr.sin_port = htons(12345);
        if(0 == connect(clients[i],(struct sockaddr*)&serverAddr,sizeof(serverAddr)))
        {
            printf("%ld connect success at clientFd %ld.\n",i,clients[i]);
        }
	}
	
	getchar();
	
	for(long i=0;i<clientLength;i++)
	{
        if(-1 != clients[i])
		{
			::close(clients[i]);
		}
	}
	
	return 0;
}
