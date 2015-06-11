#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include <errno.h>
#include <socket.h>
#include <time.h>	/* For size_t */

#include "thread.h"
#include "comm.h"

#ifdef WIN32

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

/*********************************************************************************************************/
int prepare_Send(char * addr, int port, struct sockaddr_in * server_addr){

	ServerSocket serverSocket = NULL;
	int ec;
	WSADATA wsa;
	SOCKET listen_socket = INVALID_SOCKET;
	socklen_t optlen;
	int buf_size = 163840;

	if ((ec = WSAStartup(MAKEWORD(2,0), &wsa)) != 0) {
		printf("winsock error: code %i\n");
		return -1;
	}

	if (prepareServerAddress(addr, port, server_addr) < 0){
	  	printf("error preparing address\n");
	  	return -1;
	}

	listen_socket = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);

	if (listen_socket == INVALID_SOCKET) {
		printf("socket failed with error: %i\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}
	
	optlen = sizeof(int);
	getsockopt(listen_socket,SOL_SOCKET, SO_SNDBUF, (char *) &buf_size, &optlen);
	printf("socket size %d\n", buf_size);

	setsockopt(listen_socket,SOL_SOCKET, SO_SNDBUF, (char *) &buf_size, optlen);
	setsockopt(listen_socket,SOL_SOCKET, SO_RCVBUF, (char *) &buf_size, optlen);
	
	return listen_socket;
}

/*********************************************************************************************************/
int SendT(int socketfd, void * msg, int msg_size, struct sockaddr_in * server_addr){
#ifdef DEBUG_MSGS
	printd("Sending message size %d\n", msg_size);
#endif
	if (sendto(socketfd, msg, msg_size, 0, (struct sockaddr*)server_addr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		printf("Error sending msg to IHM!\r\n");
		return -1;
	}
	return 0;
}

#else

/* FOR LINUX*/ 
#include <sys/socket.h>
#include <stddef.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/*********************************************************************************************************/
int prepare_Send(char * addr, int port, struct sockaddr_in * server_addr)
{
	int socketfd;

	/*1) Create a UDP socket*/
	if ( (socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
		printf("Error creating udp socket. Aborting!\r\n");
		return -1;
	}

	memset(server_addr, 0, sizeof(struct sockaddr_in));
	
	if (addr != NULL) {
		struct hostent *server;
		server = gethostbyname(addr);

		if (server == NULL){
		 	close(socketfd);
			printf("Error finding server name %s\r\n", addr);
			return -1;
		}

		memcpy((char *) &server_addr->sin_addr.s_addr, (char *) server->h_addr, server->h_length);
	}
	else {
		printf("null address\n");
		server_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	}

	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(port);
	
#ifdef DEBUG_MSGS
	printd("socket created %d - addr %s, port %d\n",socketfd, addr, port);
#endif
	return socketfd;
}

int SendT(int socketfd, void * msg, int msg_size, struct sockaddr_in * server_addr) {
	/*2) Establish connection*/
#ifdef DEBUG_MSGS
	printd("Sending message size %d to %d:%d\n", msg_size,server_addr->sin_port,server_addr->sin_addr.s_addr);
#endif
	if (sendto(socketfd, msg, msg_size, 0, (const struct sockaddr *)server_addr, sizeof(struct sockaddr)) < 0) {
		printf("Error sending msg to IHM!\r\n");
		return -1;
	}

	return 0;
}
#endif

/*********************************************************************************************************/
int prepare_Wait(int port)
{
	int socketfd;
	int keepalive=1;
	struct sockaddr_in servSock;

#ifdef WIN32
	int ec;
	WSADATA wsa;

	if ((ec = WSAStartup(MAKEWORD(2,0), &wsa)) != 0) {
		printf("winsock error: code %i\n");
		return -1;
	}
#endif

	/*Create a UDP socket for incoming connections*/
	if ( (socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
		printf("Error creating a udp socket port %d!\n", port);
		return -1;
	}

	memset(&servSock, 0, sizeof(servSock));
	servSock.sin_family = AF_INET;
	servSock.sin_port = htons(port);
	servSock.sin_addr.s_addr = htonl(INADDR_ANY);


	setsockopt(socketfd,SOL_SOCKET, SO_KEEPALIVE,(char *) &keepalive, sizeof(keepalive));

	/* 2) Assign a port to a socket(bind)*/
	if (bind(socketfd, (const struct sockaddr *)&servSock, sizeof(servSock) ) < 0) {
		close(socketfd);
		printf("Error on bind port %d.. %s\r\n", port, strerror(errno));
#ifdef WIN32
		WSACleanup();
#endif
		return -1;
	}

#ifdef WIN32
	BOOL bNewBehavior = FALSE;
	DWORD dwBytesReturned =0;
	if(WSAIoctl(socketfd, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL) == SOCKET_ERROR){
		printf("error set behavior socket win32\n");
	}
#endif
	return socketfd;

}

/*********************************************************************************************************/
void * WaitT(unsigned int socketfd, int timeout_ms) {
	void * buf = NULL;
	int n, ret;
	fd_set readfds;
	struct timeval timeout_sock;
	
	FD_ZERO(&readfds);
	FD_SET(socketfd, &readfds);
	timeout_sock.tv_sec = timeout_ms/1000;
	timeout_sock.tv_usec = (timeout_ms%1000)*1000;
	if(timeout_ms)
		ret = select(socketfd + 1, &readfds, NULL, NULL, &timeout_sock);
	else
		ret = select(socketfd + 1, &readfds, NULL, NULL, NULL);
	if (ret < 0) {
		printf("select error socket %d\n", socketfd);
		return NULL;
	} else if (ret == 0) {
		return NULL;
	}

	if (FD_ISSET(socketfd, &readfds)) {

		buf = malloc(2000);
		n = recv(socketfd, buf, 2000, 0);
		if (n < 0) {
			printf("ERROR reading from socket %d %d\n", socketfd, n);
#ifdef WIN32
				printf("error %d\n", WSAGetLastError());
#endif
			free(buf);
			return NULL;
		}
	}

	return buf;
}
/*********************************************************************************************************/
int prepareServerAddress(char* address, int port, struct sockaddr_in * server_addr) 
{
	memset((char *) server_addr , 0, sizeof(struct sockaddr_in));

	if (address != NULL) {
		struct hostent *server;
		server = gethostbyname(address);

		if (server == NULL) {
			printf("could not find server host by name %s\n", address);
			return -1;
		}

		memcpy((char *) &(server_addr->sin_addr.s_addr), (char *) server->h_addr, server->h_length);
	}
	else
		server_addr->sin_addr.s_addr = htonl(INADDR_ANY);

    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);

    return 0;
}
