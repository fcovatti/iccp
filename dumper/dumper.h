#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include <sys/socket.h>
#include <stddef.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#ifndef DUMPER_H_INCLUDED
#define DUMPER_H_INCLUDED

#define DEBUG_MSGS
#define PORT_DUMPER   	65280

static inline char SendT(unsigned int port, void * msg, int msg_size) {
	struct sockaddr_in servAddr;
	int socketfd;

	/*1) Create a UDP socket*/
	if ( (socketfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
		fprintf(stderr, "Error creating udp socket. Aborting!\r\n");
		return -1;
	}
	memset(&servAddr, 0, sizeof(servAddr));

	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.s_addr = INADDR_ANY;

	/*2) Establish connection*/
#ifdef DEBUG_MSGS
	printf("Sending message size %d\n", msg_size);
#endif
	if ( sendto(socketfd, msg, msg_size, 0, (const struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
			fprintf(stderr, "Error connecting!\r\n");
			return -1;
		}

	close(socketfd);	
	
	return 0;
}

static int inline prepare_Wait(unsigned long addr, int port)
{
	int socketfd;
	   struct sockaddr_in servSock;
    	/*Create a UDP socket for incoming connections*/
		if ( (socketfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
			printf("Error creating a udp socket port %d!\n", port);
			return -1;
		}

		memset(&servSock, 0, sizeof(servSock));

		servSock.sin_family = AF_INET;
		servSock.sin_port = htons(port);
		servSock.sin_addr.s_addr = htonl(addr);

		/* 2) Assign a port to a socket(bind)*/
		if ( bind(socketfd, (const struct sockaddr *)&servSock, sizeof(servSock) ) < 0) {
			close(socketfd);
			fprintf(stderr, "Error on bind port %d.. %s\r\n", port, strerror(errno));
			return -1;
		}
		printf("UDP socket initialization for port %d , address %lx, fd %d\n", port, addr, socketfd);

		return socketfd;

}
static inline void * WaitT(unsigned int socketfd, int timeout_ms) {
	void * buf = NULL;
	int n, ret;
	//fd_set masterfds;
	fd_set readfds;
	struct timeval timeout_sock;
	
	FD_ZERO(&readfds);
	FD_SET(socketfd, &readfds);
	//FD_ZERO(&masterfds);
	//FD_SET(0, &masterfds);
	//FD_ZERO(&readfds);
	//FD_SET(socketfd,&masterfds);
	//memcpy(&readfds, &masterfds, sizeof(fd_set));
	timeout_sock.tv_sec = timeout_ms/1000;
	timeout_sock.tv_usec = (timeout_ms%1000)*1000;
	//printf("waiting message socket %d\n", socketfd);
	if(timeout_ms)
		ret = select(socketfd + 1, &readfds, NULL, NULL, &timeout_sock);
	else
		ret = select(socketfd + 1, &readfds, NULL, NULL, NULL);
	//printf("waitT select ret %d\n", ret);
	if (ret < 0) {
		printf("select error socket %d\n", socketfd);
		//exit(1);
		return NULL;
	} else if (ret == 0) {
		//printf("select socket %d timeout\n", socketfd);
		return NULL;
	}
	if (FD_ISSET(socketfd, &readfds)) {

		buf = malloc(2000);
		n = recv(socketfd, buf, 2000, 0);
#ifdef DEBUG_MSGS
		printf("read %d bytes socketd %d\n", n, socketfd);
#endif
		if (n < 0) {
			printf("ERROR reading from socket %d\n", socketfd);
			free(buf);
			return NULL;
		}
	}
	return buf;
}
#endif
