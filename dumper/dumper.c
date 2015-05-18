#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include <errno.h>


#include "client.h"
#include "util.h"
#include "comm.h"

static struct sockaddr_in stats_sock_addr;

int main (int argc, char ** argv){
	int socket_dumper_send;
	int socket_dumper_receive;
	unsigned int msg_send = ICCP_STATS_SIGNATURE;
	char * msg_rcv;
	
	if (argc != 2) {
		printf("wrong usage\n stats_dump <ip iccp client>\n");
		return -1;
	}

	//preparet socket to rcv msgs
	socket_dumper_receive = prepare_Wait(PORT_STATS_TRANSMIT);

	if(socket_dumper_send < 0){
		printf("could not create UDP socket to listen to Stats Messages\n");
		return -1;
	}

	//prepare socket to send msgs
	printf("sending message to %s\n", argv[1]);
	socket_dumper_send=prepare_Send(argv[1], PORT_STATS_LISTEN, &stats_sock_addr);
	if(socket_dumper_send < 0){
		printf("could not create UDP socket to send Stats Messages\n");
		return -1;
	}

	//send message to stats server
	SendT(socket_dumper_send, (void *)&msg_send, sizeof(unsigned int), &stats_sock_addr);
	// could send argv[1]

	//waits for answer for 3 seconds
	printf("waiting reponse from %s\n", argv[1]);
	msg_rcv = WaitT(socket_dumper_receive, 3000);	

	//print return output
	if(msg_rcv == NULL) {
		printf("no msg received\n");
	} else {
		printf("msg rcv %s \n", msg_rcv);
	}

	//close socket and end program
	close(socket_dumper_send);
	close(socket_dumper_receive);
	
	return 0;
}

