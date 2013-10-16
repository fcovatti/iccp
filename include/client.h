
#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include "mms_client_connection.h"
#include "client.h"

/* Fixed defines */
// OFFSET for the index in received reports
#define INDEX_OFFSET 3

// Size of analog data in RULE 2 Information reports
#define RULE2_ANALOG_REPORT_SIZE 7

// Size of digital data in RULE 2 Information reports 
#define RULE2_DIGITAL_REPORT_SIZE 9

// name size of the standard dataset created
#define DATASET_NAME_SIZE 7

// name size of SAGE standard transferset
#define TRANSFERSET_NAME_SIZE 13

// Max Connection errors before aborting
#define MAX_CONNECTION_ERRORS 10

// Max Reading variable error in 
#define MAX_READ_ERRORS 10

// Loop in decimal seconds
#define LOOP_TIME_DS 20


/* Configurable defines */
// Time in seconds for buffering an event on a dataset before reporting to the client
#define DATASET_BUFFER_INTERVAL 1

// Time in seconds for Integrity check (configured on the remote server on initialization)
#define DATASET_INTEGRITY_TIME 10

// Max size of the dataset. IMPORTANT (bigger than 600 can cause code crash)
#define DATASET_MAX_SIZE 500

// MAx number of datasets allowed on the client
#define DATASET_MAX_NUMBER 150

// Name of the VCC on the remote server
#define IDICCP "COS_A"

// Name or IP of the remote iccp server
#define SERVER_NAME "cems1"

// Name of the configuration file
#define CONFIG_FILE "sage_id_no_155.txt"

// Name of the configuration log file
#define CONFIG_LOG "iccp_config.log"

// Name of the data received log file
#define DATA_LOG "iccp_data.log"

// Name of the error log file
#define ERROR_LOG "iccp_error.log"

typedef struct {
	unsigned int nponto;
	char id[23];
	char type;
	float f;
	unsigned char state;
	int time_stamp;
} data_config;

typedef struct {
	char id[DATASET_NAME_SIZE];
	char ts[TRANSFERSET_NAME_SIZE];
} dataset_config;

typedef union {
	float f;
	char  s[4];
} float_data;

typedef union {
	unsigned int t;
	char  s[4];
} time_data;


static inline void print_value (char state, bool ana, time_t time_stamp) {
	struct tm * time_result;
	MmsValue * value = MmsValue_newBitString(8);
	memcpy(value->value.bitString.buf, &state, 1);

	//DEBUG
	/*	printf("State_hi %d State_lo %d, Validity_hi %d, Validity_lo %d, CurrentSource_hi %d, CurrentSource_lo %d, NormalValue %d, TimeStampQuality %d \n", 
		MmsValue_getBitStringBit(value,0), MmsValue_getBitStringBit(value,1), MmsValue_getBitStringBit(value,2),
		MmsValue_getBitStringBit(value,3), MmsValue_getBitStringBit(value,4), MmsValue_getBitStringBit(value,5),
		MmsValue_getBitStringBit(value,6), MmsValue_getBitStringBit(value,7) );
	 */
	if (!ana) {
		//ESTADO
		if (MmsValue_getBitStringBit(value,0) && !MmsValue_getBitStringBit(value,1)) {
			printf("Ligado    |");
		}else if (!MmsValue_getBitStringBit(value,0) && MmsValue_getBitStringBit(value,1)) {
			printf("Desligado |");
		} else {
			printf ("Invalido |");
		}
	}

	//Validade
	if (!MmsValue_getBitStringBit(value,2) && !MmsValue_getBitStringBit(value,3)) {
		printf("Valido   |");
	}else if (!MmsValue_getBitStringBit( value,2) && MmsValue_getBitStringBit(value,3)) {
		printf("Segurado |");
	}else if (MmsValue_getBitStringBit(value,2) && !MmsValue_getBitStringBit(value,3)) {
		printf("Suspeito |");
	} else {
		printf("Inválido |");
	}

	// Origem
	if (!MmsValue_getBitStringBit(value,4) && !MmsValue_getBitStringBit(value,5)) {
		printf("Telemedido |");
	}else if (!MmsValue_getBitStringBit(value,4) && MmsValue_getBitStringBit(value,5)) {
		printf("Calculado  |");
	}else if (MmsValue_getBitStringBit(value,4) && !MmsValue_getBitStringBit(value,5)) {
		printf("Manual     |");
	} else {
		printf ("Estimado  |");
	}

	// Valor Normal
	if (!MmsValue_getBitStringBit(value,6)){
		printf ("Normal |");
	} else {
		printf ("Anormal |");
	}

	// Estampa de tempo
	if (!MmsValue_getBitStringBit(value,7)){
		printf ("T Valida |");
	} else {
		printf ("T Invalida |");
	}

	time_result = localtime(&time_stamp);
	printf("%s ", asctime(time_result));

}

#endif
