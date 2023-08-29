#ifndef SOCKET_H
#define SOCKET_H


#include "copyright.h"
#include "utility.h"
#include "sysdep.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "kernel.h"

#define MAX_FDS 20
#define MAX_OPEN_FILE_NAME 25
#define MAX_CONTENT 1000

 // array of file descriptor tables


class Socket
{
public:
    int dem = 2;
    int arrID[MAX_FDS];
    Socket() {
    
    }
    ~Socket() { 
        
    }
    int SocketTCP()
    {

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		return -1;
	}
    int index = kernel->fileSystem-> FileDescriptorFree();
    if(kernel->fileSystem->AssignFileDescriptor(index, "filename", 0, fd) != NULL)
    {
        return index;
    }

	return -1;
    }

    

    int Connect(int indexsocketid, char *ip, int port)
    {
        OpenFile *temp = kernel->fileSystem->GetFileDescriptor(indexsocketid);
        int socketid = temp->GetFile();
        if (socketid == -1)
            return -1;

        struct sockaddr_in server;
        int len = sizeof(server);

        memset(&server, '0', len);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        if (inet_pton(AF_INET, ip, &server.sin_addr) <= 0)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }

        if (connect(socketid, (struct sockaddr *)&server, len) < 0)
        {
            printf("\nConnection failed \n");
            return -1;
        }

        return 0;
    }

    int Send(int indexsocketid, char *buffer, int len)
    {
        int cnt;
        OpenFile *temp = kernel->fileSystem->GetFileDescriptor(indexsocketid);
        int socketid = temp->GetFile();

        if (socketid == -1)
            return -1;
        cnt = send(socketid, buffer, strlen(buffer), 0);
        if (cnt < 0)
            return -1;
        return cnt;
    }

    int Receive(int indexsocketid, char *buffer, int len)
    {
        
        int cnt;
        OpenFile *temp = kernel->fileSystem->GetFileDescriptor(indexsocketid);
        int socketid = temp->GetFile();

        if (socketid == -1)
            return -1;
        memset(buffer, 0, sizeof(buffer));
        cnt = read(socketid, buffer, len);
        
        if (cnt < 0)
            return -1;
        return cnt;
    }

    int CloseSocket(int indexsocketid)
    {
        bool temp = kernel->fileSystem->CloseSocketinfile(indexsocketid);
        if(temp==1)
        {
            return 0;
        }
        return -1;

    }
};

#endif SOCKET_H