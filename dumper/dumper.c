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
	stats_data_msg	msg_send;
	char * msg_rcv;
	
	stats_global_counters * gc; 
	stats_hmi_counters * hmi; 
	stats_nponto_counters * nponto_counters; 
	stats_nponto_state * nponto_state; 
	stats_cmd_counters * cmd_counters; 

	if (argc < 3 || argc > 4) {
		printf("\nWrong Usage: STATS_DUMP IPADDRESS OPTION <NPONTO>\n\n");
		printf("OPTION:\n    state <nponto>: get digital/analog/events state\n    counters <nponto>: get digital/analog/events counters\n    cmd <nponto>: get cmd stats\n    hmi: get hmi stats\n    gc: get global counters\n\n");
		return -1;
	}


	msg_send.nponto = 0;
	//get globobal counters
	if(strcmp(argv[2],"gc") == 0) {
		if (argc!=3){
			printf("wrong usage: stats_dump ipaddress gc\n");
			return -1;
		}
		msg_send.cmd = GET_GLOBAL_COUNTERS;
	} else if(strcmp(argv[2],"hmi") == 0) {
		if (argc!=3){
			printf("wrong usage: stats_dump ipaddress hmi\n");
			return -1;
		}
		msg_send.cmd = GET_HMI_COUNTERS;
	} else if(strcmp(argv[2],"state") == 0) {
		if (argc!=4){
			printf("wrong usage: stats_dump ipaddress state nponto\n");
			return -1;
		}
		msg_send.cmd = GET_NPONTO_STATE;
		msg_send.nponto = atoi(argv[3]);
	} else if(strcmp(argv[2],"counters") == 0) {
		if (argc!=4){
			printf("wrong usage: stats_dump ipaddress counters nponto\n");
			return -1;
		}
		msg_send.cmd= GET_NPONTO_COUNTERS;
		msg_send.nponto = atoi(argv[3]);
	} else if(strcmp(argv[2],"cmd") == 0) {
		if (argc!=4){
			printf("wrong usage: stats_dump ipaddress cmd nponto\n");
			return -1;
		}
		msg_send.cmd= GET_CMD_COUNTERS;
		msg_send.nponto = atoi(argv[3]);

	} else {
		printf("\nWrong Usage: STATS_DUMP IPADDRESS OPTION <NPONTO>\n\n");
		printf("OPTION:\n    state <nponto>: get digital/analog/events state\n    counters <nponto>: get digital/analog/events counters\n    cmd <nponto>: get cmd stats\n    hmi: get hmi stats\n    gc: get global counters\n\n");
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
	SendT(socket_dumper_send, (void *)&msg_send, sizeof(stats_data_msg), &stats_sock_addr);
	// could send argv[1]

	//waits for answer for 3 seconds
	printf("waiting reponse from %s\n", argv[1]);
	msg_rcv = WaitT(socket_dumper_receive, 3000);	

	//print return output
	printf("\n**********************************\n");
	if(msg_rcv == NULL) {
		printf("NO msg received\n");
	} else {
		switch (msg_send.cmd){
			case GET_GLOBAL_COUNTERS:
				gc = (stats_global_counters *) msg_rcv;
				printf("num_of_analog_ids = %d;\nnum_of_digital_ids = %d;\nnum_of_event_ids = %d;\nnum_of_commands = %d;\nnum_of_datasets = %d;\nnum_of_analog_datasets = %d;\nnum_of_digital_datasets = %d;\nnum_of_event_datasets = %d;\n", 
						gc->analog_ids,
						gc->digital_ids,
						gc->event_ids,
						gc->commands,
						gc->datasets,
						gc->analog_datasets,
						gc->digital_datasets,
						gc->event_datasets);
				break;		
			case GET_HMI_COUNTERS:
				hmi = (stats_hmi_counters *) msg_rcv;
				printf("num_of_report_msgs = %d;\nnum_of_digital_msgs = %d;\nnum_of_analog_msgs = %d;\n", 
						hmi->report_msgs,
						hmi->digital_msgs,
						hmi->analog_msgs);
				break;		
			case GET_NPONTO_COUNTERS:
				nponto_counters = (stats_nponto_counters *) msg_rcv;
				if(nponto_counters->nponto != msg_send.nponto) {
					printf("nponto not found\n");
				} else{
					printf("num_of_msg_rcv = %d;\nnum_of_reports = %d;\nnum_of_flapping = %d;\n", 
						nponto_counters->num_of_msg_rcv,
						nponto_counters->num_of_reports,
						nponto_counters->num_of_flapping);
				}
				break;		
			case GET_NPONTO_STATE:
				nponto_state = (stats_nponto_state *) msg_rcv;
				if (nponto_state->not_present)
					printf("State NOT available on the ICCP server\n");
				else {
					print_value(nponto_state->state, 0, nponto_state->time_stamp, nponto_state->time_stamp_extended, "ON", "OFF");
					printf("f=%.2f\n", nponto_state->f);
				}
				break;
			case GET_CMD_COUNTERS:
				cmd_counters = (stats_cmd_counters *) msg_rcv;
				if(cmd_counters->nponto != msg_send.nponto) {
					printf("nponto not found\n");
				} else{
					printf("num_of_msg_rcv = %d;\nnum_of_cmd_ok = %d;\nnum_of_cmd_error = %d;\n", 
						cmd_counters->num_of_msg_rcv,
						cmd_counters->num_of_cmd_ok,
						cmd_counters->num_of_cmd_error);
				}
				break;

		}
	}
	printf("**********************************\n\n");
	free(msg_rcv);
	//close socket and end program
	close(socket_dumper_send);
	close(socket_dumper_receive);
	
	return 0;
}

