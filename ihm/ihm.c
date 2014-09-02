#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include "comm.h"

static int ihm_socket_receive=0;

static unsigned int events_msgs;
static unsigned int digital_msgs;
static unsigned int analog_msgs;
static unsigned int error_msgs;
static unsigned int should_be_type_30;

static int running = 1; //used on handler for signal interruption
/*********************************************************************************************************/
static void sigint_handler(int signalId)
{
	running = 0;
}
/*********************************************************************************************************/
static int create_ihm_comm(){
	ihm_socket_receive = prepare_Wait(PORT_IHM_TRANSMIT);
	if(ihm_socket_receive < 0){
		printf("could not create UDP socket to listen to IHM\n");
		return -1;
	}
	printf("Created UDP local socket for IHM Port %d\n",PORT_IHM_TRANSMIT);
	return 0;
}

/*********************************************************************************************************/
static int check_packet(){
	char * msg_rcv;
	t_msgsup msg;
	msg_rcv = WaitT(ihm_socket_receive, 2000);	
	if(msg_rcv != NULL) {
		memcpy(&msg, msg_rcv, sizeof(t_msgsup));
		if(msg.signature==IHM_SINGLE_POINT_SIGN){
			if(msg.tipo==30)
				events_msgs++;	
			else if(msg.tipo==1){
				events_msgs++;	
				should_be_type_30++;
			}
			else
				error_msgs++;
			//	printf("wrong type %d\n",msg.tipo);
		}
		else if(msg.signature==IHM_POINT_LIST_SIGN){
			if(msg.tipo==1)
				digital_msgs++;	
			else if(msg.tipo==13)
				analog_msgs++;
			else
				error_msgs++;
			//	printf("wrong gi type %d\n",msg.tipo);
		}
		else{
			error_msgs++;
		//	printf("wrong signature %d\n", msg.signature);
		}
	}
	return -1;
}
/*********************************************************************************************************/
int main (int argc, char ** argv){
	signal(SIGINT, sigint_handler);
	if(create_ihm_comm() <0){
		printf("could not create ihm comm\n");
		return -1;
	}
	while(running){
		check_packet();
		printf("Total Receive %d - A:%d   D:%d   E:%d (%d) | Error: %d\n", (digital_msgs+analog_msgs+events_msgs),
				analog_msgs, digital_msgs, events_msgs, should_be_type_30, error_msgs);
	}
	close(ihm_socket_receive);
	return 0;
}
