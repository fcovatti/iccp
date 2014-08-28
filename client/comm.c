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

#include "comm.h"

#ifdef WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>


static bool
prepareServerAddress(char* address, int port, struct sockaddr_in * server_addr) 
{
	memset((char *) server_addr , 0, sizeof(struct sockaddr_in));

	if (address != NULL) {
		struct hostent *server;
		server = gethostbyname(address);

		if (server == NULL) return false;

		memcpy((char *) &(server_addr->sin_addr.s_addr), (char *) server->h_addr, server->h_length);
	}
	else
		server_addr->sin_addr.s_addr = htonl(INADDR_ANY);

    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);

    return true;
}

int prepare_Send(char * addr, int port, struct sockaddr_in * server_addr){

	ServerSocket serverSocket = NULL;
	int ec;
	WSADATA wsa;
	SOCKET listen_socket = INVALID_SOCKET;

	if ((ec = WSAStartup(MAKEWORD(2,0), &wsa)) != 0) {
		printf("winsock error: code %i\n");
		return -1;
	}

	if (!prepareServerAddress(addr, port, server_addr)){
	  	printf("error preparing address\n");
	  	return -1;
	}

	listen_socket = socket(AF_INET, SOCK_DGRAM,0);

	if (listen_socket == INVALID_SOCKET) {
		printf("socket failed with error: %i\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}


	/*
	int optionReuseAddr = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&optionReuseAddr, sizeof(int));
	ec = bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (ec == SOCKET_ERROR) {
		printf("bind failed with error:%i\n", WSAGetLastError());
		closesocket(listen_socket);
		WSACleanup();
		return -1;
	} */
	return listen_socket;
}

int SendT(int socketfd, void * msg, int msg_size, struct sockaddr_in * server_addr){
#ifdef DEBUG_MSGS
	printf("Sending message size %d\n", msg_size);
#endif
	if (sendto(socketfd, msg, msg_size, 0, (struct sockaddr*)server_addr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		fprintf(stderr, "Error sending msg to IHM!\r\n");
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

int prepare_Send(char * addr, int port, struct sockaddr_in * server_addr)
{
	int socketfd;

	/*1) Create a UDP socket*/
	if ( (socketfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
		fprintf(stderr, "Error creating udp socket. Aborting!\r\n");
		return -1;
	}

	memset(server_addr, 0, sizeof(struct sockaddr_in));
	
	if (addr != NULL) {
		struct hostent *server;
		server = gethostbyname(addr);

		if (server == NULL){
		 	close(socketfd);
			fprintf(stderr, "Error finding server name %s\r\n", addr);
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
	printf("socket created %d - addr %s, port %d\n",socketfd, addr, port);
#endif
	return socketfd;
}

int SendT(int socketfd, void * msg, int msg_size, struct sockaddr_in * server_addr) {
	/*2) Establish connection*/
#ifdef DEBUG_MSGS
	printf("Sending message size %d to %d:%d\n", msg_size,server_addr->sin_port,server_addr->sin_addr.s_addr);
#endif
	if (sendto(socketfd, msg, msg_size, 0, (const struct sockaddr *)server_addr, sizeof(struct sockaddr)) < 0) {
		fprintf(stderr, "Error sending msg to IHM!\r\n");
		return -1;
	}

	return 0;
}
#endif

/*********************************************************************************************************/
int prepare_Wait(char * addr, int port)
{
	int socketfd;
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
	if ( (socketfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
		printf("Error creating a udp socket port %d!\n", port);
		return -1;
	}

	memset(&servSock, 0, sizeof(servSock));

	servSock.sin_family = AF_INET;
	servSock.sin_port = htons(port);
	servSock.sin_addr.s_addr = htonl(INADDR_ANY);

	/* 2) Assign a port to a socket(bind)*/
	if ( bind(socketfd, (const struct sockaddr *)&servSock, sizeof(servSock) ) < 0) {
		close(socketfd);
		fprintf(stderr, "Error on bind port %d.. %s\r\n", port, strerror(errno));
#ifdef WIN32
	WSACleanup();
#endif
		return -1;
	}

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
/*********************************************************************************************************/
int send_analog_to_ihm(int socketfd, struct sockaddr_in * server_sock_addr,unsigned int nponto,unsigned char utr_addr, unsigned char ihm_station, float value, unsigned char state, char report){
 
			t_msgsup msg_sup;
			flutuante_seq float_value;
			memset(&msg_sup, 0, sizeof(t_msgsup));
			msg_sup.signature = 0x53535353;
			msg_sup.endereco = nponto;
			msg_sup.prim = ihm_station;
			msg_sup.sec = utr_addr;
			msg_sup.tipo = 13;

			if(report)
				msg_sup.causa=3; //report
			else
				msg_sup.causa=20; //integrity

			msg_sup.taminfo=sizeof(flutuante_seq);
			
			if(!(state&0x10) && !(state&0x20)) 
				float_value.qds=0;//valid
			else if((state&0x10) && !(state&0x20))
				float_value.qds=0x10;//held/blocked
			else
				float_value.qds=0x80;//invalid

			if((state&0x08) && !(state&0x04))
				float_value.qds = float_value.qds|0x20; //manual
			
			float_value.fr=value;
			memcpy(msg_sup.info,(char *) &float_value, sizeof(flutuante_seq));
			return SendT(socketfd,(void *)&msg_sup, sizeof(t_msgsup), server_sock_addr);
}	
/*********************************************************************************************************/
int send_digital_to_ihm(int socketfd, struct sockaddr_in * server_sock_addr,unsigned int nponto, unsigned char utr_addr, unsigned char ihm_station, unsigned char state, time_t time_stamp,unsigned short time_stamp_extended, char report){
	t_msgsup msg_sup;
	unsigned char digital_state;

	memset(&msg_sup, 0, sizeof(t_msgsup));
	msg_sup.signature = 0x53535353;
	msg_sup.endereco = nponto;
	msg_sup.prim = ihm_station;
	msg_sup.sec = utr_addr;
	
	if(!(state&0x40) && (state&0x80)) 
		digital_state=0x01;//off
	else if((state&0x40) && !(state&0x80))
		digital_state=0x00;//on
	else
		digital_state=0x80;//invalid

	if(!(state&0x10) && !(state&0x20)) 
		digital_state=digital_state|0;//valid
	else if((state&0x10) && !(state&0x20))
		digital_state=digital_state|0x10;//held/blocked
	else
		digital_state=digital_state|0x80;//invalid

	if((state&0x08) && !(state&0x04))
		digital_state=digital_state|0x20; //manual

	if(state&0x01){ 
		digital_state=digital_state|0x08; //invalid timestamp
	}

	//only send as report if timestamp is valid
	if(!(state&0x01) && report){
		struct tm * time_result;
		digital_w_time7_seq digital_value;
		msg_sup.causa=3; //report
		msg_sup.tipo=30;
		msg_sup.taminfo=sizeof(digital_w_time7_seq);
		digital_value.iq = digital_state;
		time_result = (struct tm *)localtime(&time_stamp);
		digital_value.ms=(time_result->tm_sec*1000)+time_stamp_extended;
		digital_value.min=time_result->tm_min;
		digital_value.hora=time_result->tm_hour;
		digital_value.dia=time_result->tm_mday;
		digital_value.mes=time_result->tm_mon+1;
		digital_value.ano=time_result->tm_year-100;
		memcpy(msg_sup.info,(char *) &digital_value, sizeof(digital_w_time7_seq));
	}
	else {
		digital_seq digital_value_gi;
		msg_sup.tipo=1;
		msg_sup.causa=20; //integrity
		msg_sup.taminfo=sizeof(digital_seq);
		digital_value_gi.iq = digital_state;
		memcpy(msg_sup.info,(char *) &digital_value_gi, sizeof(digital_seq));
	}

	
	return SendT(socketfd,(void *)&msg_sup, sizeof(t_msgsup), server_sock_addr);
}
/*********************************************************************************************************/
int send_cmd_response_to_ihm(int socketfd, struct sockaddr_in * server_sock_addr,unsigned int nponto, unsigned char utr_addr, unsigned char ihm_station, char cmd_ok){
	t_msgsup msg_sup;
	digital_seq digital_value_gi;
	memset(&msg_sup, 0, sizeof(t_msgsup));
	msg_sup.signature = 0x53535353;
	msg_sup.endereco = nponto;
	msg_sup.prim = ihm_station;
	msg_sup.sec = utr_addr;
	if (!cmd_ok){
		msg_sup.causa=0x43; //command response not ok
	}else{
		msg_sup.causa=0x3; //command response not ok
	}
	msg_sup.tipo=45; //IHM accepts all types
	msg_sup.taminfo=sizeof(digital_seq);
	digital_value_gi.iq = 1;
	memcpy(msg_sup.info,(char *) &digital_value_gi, sizeof(digital_seq));

	return SendT(socketfd,(void *)&msg_sup, sizeof(t_msgsup), server_sock_addr);
}
/*********************************************************************************************************/
