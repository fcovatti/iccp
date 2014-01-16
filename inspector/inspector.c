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

#include "config.h"
#include "client.h"
#include "util.h"

static int running = 1;
static void sigint_handler(int signalID){
	running = 0;
}
int main (int argc, char ** argv){
	int socket_dumper;
	unsigned int nponto;
	char * msg_rcv;
	FILE * file = NULL;
	data_analog_out analog;
	data_digital_out digital;
	signal(SIGINT, sigint_handler);

	if (argc < 2) {
		printf("Options:\n    analog: dump all analog variations\n    digital: dump all digital state changes\n    events: dump all events received\n    nponto <number>: find all changes in nponto\n");
		return -1;
	}

	if(strcmp(argv[1],"analog") == 0) {
		file = fopen(DATA_ANALOG_LOG, "r");
		if(file==NULL){
			printf("Error, cannot open data file %s\n", DATA_ANALOG_LOG);
			return -1;
		} 
		while (!feof(file) && running) {
			fread(&analog,1,sizeof(data_analog_out), file);
			printf("%d |",analog.nponto);
			printf("%f |",analog.f);
			print_value(analog.state, 1 , analog.time_stamp);
		}
		fclose(file);
	}

	if(strcmp(argv[1],"digital") == 0) {
		file = fopen(DATA_DIGITAL_LOG, "r");
		if(file==NULL){
			printf("Error, cannot open data file %s\n", DATA_DIGITAL_LOG);
			return -1;
		} 
		while (!feof(file) && running) {
			fread(&digital,1,sizeof(data_digital_out), file);
			printf("%d |",digital.nponto);
			print_value(digital.state, 1 , digital.time_stamp);
		}
		fclose(file);
	}

	if(strcmp(argv[1],"events") == 0) {
		file = fopen(DATA_EVENTS_LOG, "r");
		if(file==NULL){
			printf("Error, cannot open data file %s\n", DATA_EVENTS_LOG);
			return -1;
		} 
		while (!feof(file) && running) {
			fread(&digital,1,sizeof(data_digital_out), file);
			printf("%d |",digital.nponto);
			print_value(digital.state, 1 , digital.time_stamp);
		}
		fclose(file);
	}

	if(strcmp(argv[1],"nponto") == 0 && argc == 3) {
		nponto = atoi(argv[2]);
		printf("Searching changes in nponto %d\n", nponto);
		file = fopen(DATA_ANALOG_LOG, "r");
		if(file==NULL){
			printf("Error, cannot open data file %s\n", DATA_ANALOG_LOG);
			return -1;
		} 
		while (!feof(file) && running) {
			fread(&analog,1,sizeof(data_analog_out), file);
			if(nponto == analog.nponto) {
				printf("%d |",analog.nponto);
				printf("%f |",analog.f);
				print_value(analog.state, 1 , analog.time_stamp);
			}
		}
		fclose(file);
		file = fopen(DATA_DIGITAL_LOG, "r");
		if(file==NULL){
			printf("Error, cannot open data file %s\n", DATA_DIGITAL_LOG);
			return -1;
		} 
		while (!feof(file) && running) {
			fread(&digital,1,sizeof(data_digital_out), file);
			if(nponto == digital.nponto) {
				printf("%d |",digital.nponto);
				print_value(digital.state, 1 , digital.time_stamp);
			}
		}
		fclose(file);
		file = fopen(DATA_EVENTS_LOG, "r");
		if(file==NULL){
			printf("Error, cannot open data file %s\n", DATA_EVENTS_LOG);
			return -1;
		} 
		while (!feof(file) && running) {
			fread(&digital,1,sizeof(data_digital_out), file);
			if(nponto == digital.nponto) {
				printf("%d |",digital.nponto);
				print_value(digital.state, 1 , digital.time_stamp);
			}		}
		fclose(file);
	}
	return 0;
}
