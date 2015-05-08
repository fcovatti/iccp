#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
//#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include "mms_client_connection.h"
#include "util.h"
#include "client.h"
extern Semaphore localtime_mutex;
#ifdef WIN32
static struct tm *localtime_r(const time_t *tloc, struct tm *result) {
	struct tm *tm;
	if((tm = localtime(tloc)))
		return memcpy(result, tm, sizeof(struct tm));
	return 0;
}
#endif

/*********************************************************************************************************/
void write_dataset(MmsConnection con, char * id_iccp, char * ds_name, char * ts_name, int buffer_time, int integrity_time, int all_changes_reported)
{
	//global
	MmsError mmsError;
	MmsVariableSpecification * typeSpec= calloc(1,sizeof(MmsVariableSpecification));
	typeSpec->type = MMS_STRUCTURE;
	typeSpec->typeSpec.structure.elementCount = 13;
	typeSpec->typeSpec.structure.elements = calloc(13,
			sizeof(MmsVariableSpecification*));

	MmsVariableSpecification* element;

	//0
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->type = MMS_STRUCTURE;
	element->typeSpec.structure.elementCount = 3;
	element->typeSpec.structure.elements = calloc(3,
			sizeof(MmsVariableSpecification*));

	MmsVariableSpecification* inside_element;
	//0.0
	inside_element = calloc(1, sizeof(MmsVariableSpecification));
	inside_element->type = MMS_UNSIGNED;
	inside_element->typeSpec.unsignedInteger = 8;
	inside_element->typeSpec.structure.elementCount = 3;
	element->typeSpec.structure.elements[0] = inside_element;

	//0.1
	inside_element = calloc(1, sizeof(MmsVariableSpecification));
	inside_element->type = MMS_VISIBLE_STRING;
	inside_element->typeSpec.visibleString = -129;
	element->typeSpec.structure.elements[1] = inside_element;

	//0.2
	inside_element = calloc(1, sizeof(MmsVariableSpecification));
	inside_element->type = MMS_VISIBLE_STRING;
	inside_element->typeSpec.visibleString = -129;
	element->typeSpec.structure.elements[2] = inside_element;

	typeSpec->typeSpec.structure.elements[0] = element;

	//1
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec->typeSpec.structure.elements[1] = element;

	//2
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec->typeSpec.structure.elements[2] = element;

	//3
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec->typeSpec.structure.elements[3] = element;

	//4
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec->typeSpec.structure.elements[4] = element;

	//5
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec->typeSpec.structure.elements[5] = element;

	//6
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->typeSpec.bitString = 5;
	element->type = MMS_BIT_STRING;
	typeSpec->typeSpec.structure.elements[6] = element;

	//7
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec->typeSpec.structure.elements[7] = element;

	//8
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec->typeSpec.structure.elements[8] = element;

	//9
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec->typeSpec.structure.elements[9] = element;

	//10
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec->typeSpec.structure.elements[10] = element;

	//11
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec->typeSpec.structure.elements[11] = element;

	//12
	element = calloc(1, sizeof(MmsVariableSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec->typeSpec.structure.elements[12] = element;

	MmsValue* dataset = MmsValue_newStructure(typeSpec);

	//0
	MmsValue* elem;
	elem = MmsValue_getElement(dataset, 0);

	//0.0
	MmsValue* ielem;
	ielem = MmsValue_getElement(elem, 0);
	MmsValue_setUint8(ielem, 1);

	//0.1
	ielem = MmsValue_getElement(elem, 1);
	MmsValue_setVisibleString(ielem, id_iccp);

	//0.2
	ielem = MmsValue_getElement(elem, 2);
	MmsValue_setVisibleString(ielem, ds_name);

	//1
	elem = MmsValue_getElement(dataset, 1);
	MmsValue_setInt32(elem, 0);

	//2
	elem = MmsValue_getElement(dataset, 2);
	MmsValue_setInt32(elem, 0);

	//3
	elem = MmsValue_getElement(dataset, 3);
	MmsValue_setInt32(elem, 0);

	//4
	//Buffer interval
	elem = MmsValue_getElement(dataset, 4);
	MmsValue_setInt32(elem, buffer_time);

	//5
	//Integrity check time
	elem = MmsValue_getElement(dataset, 5);
	MmsValue_setInt32(elem, integrity_time);

	// 6
	elem = MmsValue_getElement(dataset, 6);
	MmsValue_setBitStringBit(elem, 1, true);
	MmsValue_setBitStringBit(elem, 2, true);

	//7
	elem = MmsValue_getElement(dataset, 7);
	MmsValue_setBoolean(elem, true);

	//8
	elem = MmsValue_getElement(dataset, 8);
	MmsValue_setBoolean(elem, true);

	//9
	elem = MmsValue_getElement(dataset, 9);
	MmsValue_setBoolean(elem, true);

	//10
	//RBE?
	//FIXME: if true send notification if lost event
	elem = MmsValue_getElement(dataset, 10);
	if(all_changes_reported)
		MmsValue_setBoolean(elem, true);
	else
		MmsValue_setBoolean(elem, false);
	//MmsValue_setBoolean(elem, true);

	//11
	elem = MmsValue_getElement(dataset, 11);
	MmsValue_setBoolean(elem, true);

	//12
	elem = MmsValue_getElement(dataset, 12);
	MmsValue_setInt32(elem, 0);

	MmsConnection_writeVariable(con, &mmsError, id_iccp, ts_name, dataset );

	MmsVariableSpecification_destroy(typeSpec);
	MmsValue_delete(dataset);
}

/*********************************************************************************************************/
MmsValue * get_next_transferset(MmsConnection con, char * id_iccp)
{
//READ DATASETS
	MmsError mmsError;
	MmsValue* returnValue = NULL;
	MmsValue* value;
	value = MmsConnection_readVariable(con, &mmsError, id_iccp, "Next_DSTransfer_Set");
	
	if (value == NULL){                                                                                                                
		LOG_MESSAGE( "ERROR - reading transfer set value failed! %d\n", mmsError);                                                                                                   
		return NULL;
	}
	MmsValue* ts_elem;
	ts_elem = MmsValue_getElement(value, 0);
	if (ts_elem == NULL){
		LOG_MESSAGE( "ERROR - reading transfer set value 0 failed! %d\n", mmsError);                                                                                                   
		return NULL;
	}

	ts_elem = MmsValue_getElement(value, 1);
	if (ts_elem == NULL){
		LOG_MESSAGE( "ERROR - reading transfer set value 1 failed! %d\n", mmsError);                                                                                                   
		return NULL;
	} else {
		//printd("Read transfer set domain_id: %s\n", MmsValue_toString(ts_elem));
		if(strncmp(MmsValue_toString(ts_elem), id_iccp, sizeof(id_iccp)) != 0){
			LOG_MESSAGE( "ERROR - Wrong domain id\n");
			return NULL;
		}
	}

	ts_elem = MmsValue_getElement(value, 2);
	if (ts_elem == NULL){
		LOG_MESSAGE( "ERROR - reading transfer set value 2 failed! %d\n", mmsError);                                                                                                   
		return NULL;
	} else {
		//printd("Read transfer set name: %s\n", MmsValue_toString(ts_elem));
		returnValue = MmsValue_newMmsString(MmsValue_toString(ts_elem));
	}
	MmsValue_delete(value); 
	return returnValue;
}

/*********************************************************************************************************/

int check_connection(MmsConnection con, char * id_iccp, int * loop_error) {
	MmsError mmsError;
	MmsValue* loop_value;

	loop_value = MmsConnection_readVariable(con, &mmsError, id_iccp, "Bilateral_Table_ID");
	if (loop_value == NULL){ 
		LOG_MESSAGE( "WARNING - loop value == NULL \n");
		if (mmsError == MMS_ERROR_CONNECTION_LOST){
			LOG_MESSAGE( "ERROR - Connection lost\n");
			return -1;
		}
		else if (mmsError == MMS_ERROR_SERVICE_TIMEOUT){
			*loop_error++;
			LOG_MESSAGE( " WARN - timeout resposta - %d\n", *loop_error);
			if(*loop_error < MAX_READ_ERRORS){
				LOG_MESSAGE("WARN - Read Service Timeout %d - mmsError %d\n", *loop_error, mmsError);
			}else {
				LOG_MESSAGE( "aborting due to consecutive Read Service Timeouts\n");
				LOG_MESSAGE( "ERROR - aborting due to consecutive Read Service Timeouts\n");
				return -1;
			}
		} else if (mmsError == MMS_ERROR_NONE) {
			*loop_error++;
			LOG_MESSAGE( "WARN - loop_erro %d\n", *loop_error);
			if(*loop_error < MAX_READ_ERRORS){
				LOG_MESSAGE("WARN - loop error %d reading value - mmsError %d\n", *loop_error, mmsError);
			}else {
				LOG_MESSAGE( "ERROR - aborting due to consecutive errors reading value\n");
				return -1;
			}
		} else {
			LOG_MESSAGE( " ERROR - Unkwnon read error %d error. Aborting!\n", mmsError);
			return -1;
		}
	}else{                                                                                                                                     
		MmsValue_delete(loop_value);
		*loop_error=0;
	}
	return 0;
}

/*********************************************************************************************************/
int connect_to_iccp_server(MmsConnection * con, char * srv_1, char *srv_2, char *srv_3, char *srv_4){
	if ((connect_to_server(*con, srv_1) < 0)){
		MmsConnection_destroy(*con);
		*con = MmsConnection_create();
		if ((connect_to_server(*con, srv_2) < 0)){
			MmsConnection_destroy(*con);
			*con = MmsConnection_create();
			if ((connect_to_server(*con, srv_3) < 0)){
				MmsConnection_destroy(*con);
				*con = MmsConnection_create();
				if ((connect_to_server(*con, srv_4) < 0)){
					MmsConnection_destroy(*con);
					*con = MmsConnection_create();
					return -1;
				}
			}
		}
	}
	return 0;
}
/*********************************************************************************************************/
int connect_to_server(MmsConnection con, char * server)
{

	MmsError mmsError;

    if(strcmp(server,"no")==0) {
		//FIXME dont go till here to check if server is disabled with "no"
		return -1;
	}
	
	bool indication = MmsConnection_connect(con, &mmsError, server, 102);
	if (indication){
		LOG_MESSAGE("Connection OK - server %s !!!\n", server);
		return 0;
	}
	LOG_MESSAGE("Connection Error %d %d - server %s\n", indication, mmsError, server);
	return -1;
}

/*********************************************************************************************************/
int command_variable(MmsConnection con, char * variable, int value)
{
	MmsError mmsCmdError;
	char select_before_operate[MAX_STR_NAME];
	snprintf(select_before_operate, MAX_STR_NAME, "%s_SBO", variable);

	MmsValue*	readvalue =
		MmsConnection_readVariable(con, &mmsCmdError, NULL, select_before_operate);

	if (readvalue == NULL){
		LOG_MESSAGE( "ERROR - could not read command %s \n", select_before_operate);
		return -1;
	}
	else{
		//printd("Read %s SBO value %d\n",variable, MmsValue_toInt32(readvalue));
		//MmsValue_setInt32(readvalue, 0);
		MmsValue_delete(readvalue);
		MmsValue* writeValue= MmsValue_newIntegerFromInt32(value);
		MmsConnection_writeVariable(con, &mmsCmdError, NULL, variable, writeValue);
		MmsValue_delete(writeValue);
		if(mmsCmdError != MMS_ERROR_NONE){
			LOG_MESSAGE( "ERROR - could not write command %s, value %d, mmsError %d \n", variable, value, mmsCmdError);
			return -1;
		} else{
			LOG_MESSAGE( "INFO - write command %s, value %d \n", variable, value);
		}
	}
	return 0;
}

/*********************************************************************************************************/
void print_time(FILE * log_file){
	time_t t = time(NULL);
	//struct tm now = *localtime(&t); 
	struct tm now = {0};

	Semaphore_wait(localtime_mutex);	
   	localtime_r(&t, &now ); 
	Semaphore_post(localtime_mutex);	
	fprintf(log_file, "%04d/%02d/%02d %02d:%02d:%02d - ", now.tm_year+1900, now.tm_mon+1, now.tm_mday,now.tm_hour, now.tm_min,now.tm_sec);
}
