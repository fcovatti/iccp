
#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <time.h>
#include "mms_types.h"
#include "mms_value_internal.h"
#include "mms_client_connection.h"

void write_dataset(MmsConnection con, char * ds_name, char * ts_name, int buffer_time, int integrity_time, int all_changes_reported);

MmsValue * get_next_transferset(MmsConnection con, FILE * error_file);

int check_connection(MmsConnection con, FILE *    error_file);

int connect_to_server(MmsConnection con, char * server);

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
		printf ("Normal  |");
	} else {
		printf ("Anormal |");
	}

	// Estampa de tempo
	if (!MmsValue_getBitStringBit(value,7)){
		printf ("T Valida   |");
	} else {
		printf ("T Invalida |");
	}

	time_result = localtime(&time_stamp);
	printf("%s ", asctime(time_result));

}

#endif
