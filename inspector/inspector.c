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

#include <libconfig.h>
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
	config_t cfg;
	const char *str1;
	const char *cfg_file;

	signal(SIGINT, sigint_handler);

	if (argc < 2) {
		printf("Options:\n    analog: dump all analog variations\n    digital: dump all digital state changes\n    events: dump all events received\n    nponto <number>: find all changes in nponto\n");
		return -1;
	}

	/*****************
	 * READ ICCP CONFIGURATION PARAMETERS
	 **********/

	config_init(&cfg);

	if(!config_read_file(&cfg, ICCP_CLIENT_CONFIG_FILE)){
		printf("\n %s %d", config_error_file(&cfg), config_error_line(&cfg));
		config_destroy(&cfg);
		return -1;
	}
	//CONFIG_FILE
	if(config_lookup_string(&cfg, "CONFIG_FILE", &cfg_file)){
		printf("CONFIG_FILE %s\n", cfg_file);
	}
	else{
	   printf("\n no CONFIG_FILE on the configuration file\n");
		config_destroy(&cfg);
		return -1;
	}

	printf(" "); //just for adjusting padding of first printed value
	if(strcmp(argv[1],"analog") == 0) {
		file = fopen(DATA_ANALOG_LOG, "r");
		if(file==NULL){
			printf("Error, cannot open data file %s\n", DATA_ANALOG_LOG);
			return -1;
		} 
		while (!feof(file) && running) {
			fread(&analog,1,sizeof(data_analog_out), file);
			printf("%7d |",analog.nponto);
			printf("%10.2f |",analog.f);
			print_value(analog.state, 1 , analog.time_stamp, "","");
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
			printf("%7d |",digital.nponto);
			print_value(digital.state, 0 , digital.time_stamp, "","");
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
			printf("%7d |",digital.nponto);
			print_value(digital.state, 0 , digital.time_stamp, "","");
		}
		fclose(file);
	}

	if(strcmp(argv[1],"nponto") == 0 && argc == 3) {
		char state_off[16] = "";
		char state_on[16] = "";
		char id_ponto[25] = "";
		nponto = atoi(argv[2]);
		printf("Searching changes in nponto %d\n\n", nponto);
		
		// print sage_id description
		file = fopen(cfg_file, "r");
		if(file==NULL){
			printf("Error, cannot open configuration file %s\n", cfg_file);
			return -1;
		} else{
			char line[300];
			unsigned int nponto_file = 0;
			char valores[35];
			char descritivo[45];
			//first two rows of cfg_file are the reader, discard them
			if(!fgets(line, 300, file) || !fgets(line, 300, file)){
				printf("Error reading %s file header\n", cfg_file);
				return -1;
			}
			while ( fgets(line, 300, file)){
				if(sscanf(line, "%d %*d %22s %*c %31s %*d %*d %*d %*d %*c %*d %*d %*f %*f %*d %*d %*d %*f %44c", &nponto_file, id_ponto,  valores, descritivo ) <1)
				//if(sscanf(line, "%d ", &nponto_file) <1)
					break;
				if (nponto == nponto_file){
					//nponto found in sage_id
					int i = 0;
					int value_split = 0;
					for ( i=0; i <35; i++) {
						if (valores[i] == '/' ){
							value_split=i;
							state_on[i]=0;
							continue;
						}
						if(value_split){
							if (valores[i] == 0 ) {
								state_off[i-value_split-1] =0;
								break;
							}else
								state_off[i-value_split-1] = valores[i];
						}else
							state_on[i] = valores[i];

					}
					break;
				}
			}
		}
		fclose(file);
		config_destroy(&cfg);

		printf(" ");
		file = fopen(DATA_ANALOG_LOG, "r");
		if(file==NULL){
			printf("Error, cannot open data file %s\n", DATA_ANALOG_LOG);
			return -1;
		} 
		while (!feof(file) && running) {
			fread(&analog,1,sizeof(data_analog_out), file);
			if(nponto == analog.nponto) {
				printf(" %7d - %25s |", analog.nponto, id_ponto);
				printf("%11.2f %-6s |",analog.f, state_on);
				print_value(analog.state, 1 , analog.time_stamp, "","");
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
				printf(" %7d - %25s |", digital.nponto, id_ponto);
				print_value(digital.state, 0 , digital.time_stamp, state_on, state_off);
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
				printf(" %7d - %25s |", digital.nponto, id_ponto);
				print_value(digital.state, 0 , digital.time_stamp, state_on, state_off);
			}
		}
		fclose(file);

	}
	return 0;
}
