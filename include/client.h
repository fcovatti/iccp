
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


#define INDEX_OFFSET 3
#define RULE2_ANALOG_REPORT_SIZE 7
#define RULE2_DIGITAL_REPORT_SIZE 9
#define DATASET_MAX_SIZE 500
#define DATASET_MAX_NUMBER 150
#define DATASET_NAME_SIZE 7
#define DATASET_BUFFER_INTERVAL 1
#define DATASET_INTEGRITY_TIME 2
#define TRANSFERSET_NAME_SIZE 13

#define IDICCP "COS_A"
#define SERVER_NAME "cems1"
#define CONFIG_FILE "sage_id_no_155.txt"
#define CONFIG_LOG "iccp_config.log"
#define DATA_LOG "iccp_data.log"

typedef struct {
	unsigned int nponto;
	char id[23];
	char type;
	float f;
	char  state;
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
