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

#include "dumper.h"
static int running = 1;
static void sigint_handler(int signalID){
	running = 0;
}
int main (int argc, char ** argv){
	int socket_dumper;
	char * msg_rcv;
	signal(SIGINT, sigint_handler);
	if (argc != 2) {
		printf("wrong usage\n");
		return -1;
	}
	if( atoi(argv[1]) == 1) {
		socket_dumper = prepare_Wait(INADDR_ANY, PORT_DUMPER);
		while(running) {
			msg_rcv = WaitT(socket_dumper, 30000);	
			if(msg_rcv == NULL) {
				printf("no msg received\n");
			} else {
				printf("msg rcv %s \n", msg_rcv);
			}
		}
		close(socket_dumper);
	}else {
		SendT(PORT_DUMPER, argv[1], 1);
	}
	return 0;
}
