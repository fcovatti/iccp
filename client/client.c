#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include "mms_client_connection.h"
#include "mms_value_internal.h"
#include "client.h"
#include "comm.h"
#include "util.h"
#include "control.h"
#include "socket.h"

/***************************** Defines for code debugging********************************************/
//#define HANDLE_DIGITAL_DATA_DEBUG 1
//#define HANDLE_ANALOG_DATA_DEBUG 1
//#define HANDLE_EVENTS_DATA_DEBUG 1
//#define REPORTS_ONLY_DATA_DEBUG 1
//#define DEBUG_READ_DATASET 1
//#define DEBUG_DIGITAL_REPORTS 1
//#define DEBUG_EVENTS_REPORTS 1
//#define DATA_LOG 1
/*********************************************************************************************************/

static int running = 1; //used on handler for signal interruption

/***************************************Configuration**************************************************/
static int num_of_analog_ids = 0;	
static int num_of_digital_ids = 0;	
static int num_of_event_ids = 0;	
static int num_of_commands = 0;	

static int num_of_datasets = 0;
static int num_of_analog_datasets = 0;
static int num_of_digital_datasets = 0;
static int num_of_event_datasets = 0;

static data_config * analog = NULL;
static data_config * digital = NULL;
static data_config * events = NULL;
static command_config * commands = NULL;

static dataset_config * dataset_conf = NULL;

static char IDICCP[MAX_ID_ICCP_NAME];

static char srv1[MAX_STR_NAME], srv2[MAX_STR_NAME], srv3[MAX_STR_NAME], srv4[MAX_STR_NAME]; 
static char srv5[MAX_STR_NAME], srv6[MAX_STR_NAME], srv7[MAX_STR_NAME], srv8[MAX_STR_NAME]; 
static int analog_gi=0, digital_gi=0, events_gi=0, analog_buf=0, digital_buf=0, events_buf=0;

static char ihm_addr[MAX_STR_NAME];
static int ihm_socket_send=0;
static int ihm_socket_receive=0;
static int ihm_enabled=0;
static int ihm_station=0;
static struct sockaddr_in ihm_sock_addr;
/*********************************************************************************************************/

#ifdef DATA_LOG
static FILE * data_file_analog = NULL;
static FILE * data_file_digital = NULL;
static FILE * data_file_events = NULL;
#endif

static FILE * error_file = NULL;

/***********************************Handle functions*****************************************************/
static int handle_analog_state(float value, unsigned char state, unsigned int index,time_t time_stamp, char report);
static int handle_digital_state(unsigned char state, unsigned int index, time_t time_stamp, unsigned short time_stamp_extended, char report);
static int handle_event_state(unsigned char state, unsigned int index, time_t time_stamp, unsigned short time_stamp_extended, char report);

/*********************************************************************************************************/
void handle_analog_report(float value, unsigned char state, unsigned int index,time_t time_stamp){
	handle_analog_state(value,state,index,time_stamp,1);
}

void handle_digital_report(unsigned char state, unsigned int index,time_t time_stamp, unsigned short time_stamp_extended){
	handle_digital_state(state, index, time_stamp,time_stamp_extended,1);
}

void handle_event_report(unsigned char state, unsigned int index,time_t time_stamp, unsigned short time_stamp_extended){
	handle_event_state(state, index, time_stamp,time_stamp_extended,1);
}

void handle_analog_integrity(float value, unsigned char state, unsigned int index,time_t time_stamp){
#ifndef REPORTS_ONLY_DATA_DEBUG
	handle_analog_state(value,state,index,time_stamp,0);
#endif
}
void handle_digital_integrity(unsigned char state, unsigned int index,time_t time_stamp, unsigned short time_stamp_extended){
#ifndef REPORTS_ONLY_DATA_DEBUG
	handle_digital_state(state, index, time_stamp,time_stamp_extended,0);
#endif
}

void handle_event_integrity(unsigned char state, unsigned int index,time_t time_stamp, unsigned short time_stamp_extended){
#ifndef REPORTS_ONLY_DATA_DEBUG
	handle_event_state(state, index, time_stamp,time_stamp_extended,0);
#endif
}

/*********************************************************************************************************/

static int handle_analog_state(float value, unsigned char state, unsigned int index,time_t time_stamp,char report){

	if (analog != NULL && index >=0 && index < num_of_analog_ids) {

#ifdef DATA_LOG
		if((analog[index].f != value) || (analog[index].state != state)){
			data_analog_out data;
			data.nponto = analog[index].nponto;
			data.state = state;
			data.time_stamp = time_stamp;
			data.f = value;
			fwrite(&data,1,sizeof(data_analog_out),data_file_analog);
		}
#endif
		analog[index].f = value;
		analog[index].state = state;
		analog[index].time_stamp = time_stamp;

#ifdef HANDLE_ANALOG_DATA_DEBUG	
		printf("%25s: %11.2f %-6s |", analog[index].id, value,analog[index].state_on);
		print_value(state,1, time_stamp,0, "", "");
#endif
		if(ihm_enabled && ihm_socket_send > 0){
			send_analog_to_ihm(ihm_socket_send, &ihm_sock_addr, analog[index].nponto, analog[index].utr_addr,ihm_station, value, state, report, 0);
		}
	}
	return 0;
}
/*********************************************************************************************************/

static int handle_digital_state(unsigned char state, unsigned int index, time_t time_stamp, unsigned short time_stamp_extended,char report){

	if (digital != NULL && index >=0 && index < num_of_digital_ids) {

#ifdef DATA_LOG
		if((digital[index].state != state)){
			data_digital_out data;
			data.nponto = digital[index].nponto;
			data.state = state;
			data.time_stamp = time_stamp;
			data.time_stamp_extended = time_stamp_extended;
			fwrite(&data,1,sizeof(data_digital_out),data_file_digital);
		}
#endif
		digital[index].state = state;
		digital[index].time_stamp = time_stamp;
		digital[index].time_stamp_extended = time_stamp_extended;

#ifdef DEBUG_DIGITAL_REPORTS	
		if(report){
			printf("%25s: ", digital[index].id);
			print_value(state,0, time_stamp, time_stamp_extended, digital[index].state_on, digital[index].state_off);
		}
#endif

#ifdef HANDLE_DIGITAL_DATA_DEBUG	
		printf("%25s: ", digital[index].id);
		print_value(state,0, time_stamp, time_stamp_extended, digital[index].state_on, digital[index].state_off);
#endif
		if(ihm_enabled && ihm_socket_send > 0){
			send_digital_to_ihm(ihm_socket_send, &ihm_sock_addr, digital[index].nponto, digital[index].utr_addr, ihm_station, state, time_stamp, time_stamp_extended, report, 0);
		}
	}
	return 0;
}

/*********************************************************************************************************/

static int handle_event_state(unsigned char state, unsigned int index, time_t time_stamp, unsigned short time_stamp_extended, char report){

	if (events != NULL && index >=0 && index < num_of_event_ids) {

#ifdef DATA_LOG
		if((events[index].state != state)){
			data_digital_out data;
			data.nponto = events[index].nponto;
			data.state = state;
			data.time_stamp = time_stamp;
			data.time_stamp_extended = time_stamp_extended;
			fwrite(&data,1,sizeof(data_digital_out),data_file_events);
		}
#endif
		events[index].state = state;
		events[index].time_stamp = time_stamp;
		events[index].time_stamp_extended = time_stamp_extended;


#ifdef DEBUG_EVENTS_REPORTS	
		if(report){
			printf("%25s: ", events[index].id);
			print_value(state,0, time_stamp, time_stamp_extended, events[index].state_on, events[index].state_off);
		}
#endif

#ifdef HANDLE_EVENTS_DATA_DEBUG	
		printf("%25s: ", events[index].id);
		print_value(state,0, time_stamp, time_stamp_extended, events[index].state_on, events[index].state_off);
#endif
		if(ihm_enabled && ihm_socket_send > 0){
			send_digital_to_ihm(ihm_socket_send, &ihm_sock_addr, events[index].nponto, events[index].utr_addr, ihm_station, state, time_stamp, time_stamp_extended,report, 0);
		}
	}
	return 0;
}

/*********************************************************************************************************/

static int read_dataset(MmsConnection con, char * ds_name, unsigned int offset){
	MmsValue* dataSet;
	MmsError mmsError;
	int i, idx;
	MmsValue* dataSetValue;
	MmsValue* dataSetElem;
	MmsValue* timeStamp;
	time_t time_stamp;
#ifdef DEBUG_READ_DATASET	
	char debug_read[50];
	int debug_i;
#endif
	int number_of_variables = dataset_conf[offset].size; 
	unsigned char data_state =0;
	unsigned short time_stamp_extended =0;
	unsigned char * ts_extended;
	ts_extended = (char *) &time_stamp_extended;

	dataSet = MmsConnection_readNamedVariableListValues(con, &mmsError, IDICCP, ds_name, 0);
	if (dataSet == NULL){
		fprintf(error_file,"ERROR - reading dataset failed! %d\n", mmsError);                                                                                                   
		fflush(error_file);
		return -1;
	}else{
		for (i=0; i < number_of_variables; i ++) {
			idx = i+dataset_conf[offset].offset;

			//GET DATASET VALUES (FIRST 3 VALUES ARE ACCESS-DENIED)
			dataSetValue = MmsValue_getElement(dataSet, INDEX_OFFSET+i);
			
#ifdef DEBUG_READ_DATASET	
			memset(debug_read,0,50);
			MmsValue_printToBuffer(dataSetValue, debug_read, 50);
			for (debug_i=0;debug_i<50; debug_i++){
				printf("%X", debug_read[debug_i]);
			}
			printf("\n");
#endif			
			if(dataSetValue == NULL) {
				fprintf(error_file, "ERROR - could not get DATASET values offset %d element %d %d \n",offset, idx, number_of_variables);
				fflush(error_file);
				//TODO return -1;
				continue;
			}

			if(dataset_conf[offset].type == DATASET_ANALOG){
				MmsValue* analog_value;
				float analog_data = 0.0;
				data_state = 0;

				//First element Floating Point Value
				analog_value = MmsValue_getElement(dataSetValue, 0);
				if(analog_value == NULL) {
					fprintf(error_file, "ERROR - could not get floating point value %s - nponto %d\n", analog[idx].id, analog[idx].nponto);
					fflush(error_file);
					analog[idx].not_present=1;
					//TODO if non existent object return -1;
				}else { 
					analog_data = MmsValue_toFloat(analog_value);
					//Second Element BitString data state
					analog_value = MmsValue_getElement(dataSetValue, 1);
					if(analog_value == NULL) {
						fprintf(error_file, "ERROR - could not get analog state %s - nponto %d\n", analog[idx].id, analog[idx].nponto);
						fflush(error_file);
						//TODO if non existent object return -1;
					} else {
						data_state = analog_value->value.bitString.buf[0];
					}
					time( &time_stamp);
					handle_analog_integrity(analog_data, data_state,idx , time_stamp);
				}
			} else if(dataset_conf[offset].type == DATASET_DIGITAL){
				data_state = 0;
				//First element Data_TimeStampExtended
				dataSetElem = MmsValue_getElement(dataSetValue, 0);
				if(dataSetElem == NULL) {
					fprintf(error_file, "ERROR - could not get digital Data_TimeStampExtended %s - nponto %d\n", digital[idx].id, digital[idx].nponto);
					fflush(error_file);
					time(&time_stamp); //use system time stamp
					time_stamp_extended=0;
					digital[idx].not_present=1;
					//TODO if non existent object return -1;
				}else{
					//printf("nponto %d extended %x %x \n", digital[idx].nponto, dataSetElem->value.octetString.buf[0], dataSetElem->value.octetString.buf[1]);	
					ts_extended[1]= dataSetElem->value.octetString.buf[0];
					ts_extended[0]= dataSetElem->value.octetString.buf[1];
					timeStamp = MmsValue_getElement(dataSetElem, 0);
					if(timeStamp == NULL) {
						fprintf(error_file, "ERROR - could not get digital timestamp value %s - nponto %d\n", digital[idx].id, digital[idx].nponto);
						fflush(error_file);
						//TODO if non existent object return -1;
					}else{
						time_stamp = MmsValue_toUint32(timeStamp);
					}
				
					//Second Element DataState
					dataSetElem = MmsValue_getElement(dataSetValue, 1);
					if(dataSetElem == NULL) {
						fprintf(error_file, "ERROR - could not get digital DataState %s - nponto %d\n", digital[idx].id, digital[idx].nponto);
						fflush(error_file);
						//TODO if non existent object return -1;
					}else {
						data_state = dataSetElem->value.bitString.buf[0];
					}
					handle_digital_integrity(data_state, idx, time_stamp, time_stamp_extended); 
				}
			} else if(dataset_conf[offset].type == DATASET_EVENTS){
				data_state = 0;
				//First element Data_TimeStampExtended
				dataSetElem = MmsValue_getElement(dataSetValue, 0);
				if(dataSetElem == NULL) {
					fprintf(error_file, "ERROR - could not get event Data_TimeStampExtended %s - nponto %d\n", events[idx].id, events[idx].nponto);
					fflush(error_file);
					time(&time_stamp); //use system time stamp
					time_stamp_extended=0;
					events[idx].not_present=1;
					//TODO if non existent object return -1;
				}else{
					ts_extended[1]= dataSetElem->value.octetString.buf[0];
					ts_extended[0]= dataSetElem->value.octetString.buf[1];
					
					timeStamp = MmsValue_getElement(dataSetElem, 0);

					if(timeStamp == NULL) {
						fprintf(error_file, "ERROR - could not get event timestamp value %s - nponto %d\n", events[idx].id, events[idx].nponto);
						fflush(error_file);
						//return -1;
					}
					time_stamp = MmsValue_toUint32(timeStamp);
					
					//Second Element DataState
					dataSetElem = MmsValue_getElement(dataSetValue, 1);
					if(dataSetElem == NULL) {
						fprintf(error_file, "ERROR - could not get event DataState %s - nponto %d\n", events[idx].id, events[idx].nponto);
						fflush(error_file);
						//TODO if non existent object return -1;
					}else {
						data_state = dataSetElem->value.bitString.buf[0];
					}
					handle_event_integrity(data_state, idx, time_stamp,time_stamp_extended); 		
				}
			} else{
				fprintf(error_file, "ERROR - unknown configuration type for dataset %d offset %d\n", offset, idx);
				fflush(error_file);
				return -1;
			}
			//MmsValue_delete(dataSetValue); 
		}
	}
	//after reading dataset flush files
#if DATA_LOG
	switch(dataset_conf[offset].type) {
		case DATASET_ANALOG:
			fflush(data_file_analog);	
			break;
		case DATASET_DIGITAL:
			fflush(data_file_digital);
			break;
		case DATASET_EVENTS:
			fflush(data_file_events);
			break;
	}
#endif

	MmsValue_delete(dataSet); 
	return 0;
}

/*********************************************************************************************************/

static void create_dataset(MmsConnection con, char * ds_name, int offset)
{
	MmsError mmsError;
	int i=0;
	int first = 0;
	int last = 0;
	LinkedList variables = LinkedList_create();

	MmsVariableAccessSpecification * var; 
	MmsVariableAccessSpecification * name = MmsVariableAccessSpecification_create (IDICCP, "Transfer_Set_Name");
	MmsVariableAccessSpecification * ts   = MmsVariableAccessSpecification_create (IDICCP, "Transfer_Set_Time_Stamp");
	MmsVariableAccessSpecification * ds   = MmsVariableAccessSpecification_create (IDICCP, "DSConditions_Detected");
	LinkedList_add(variables, name );
	LinkedList_add(variables, ts);
	LinkedList_add(variables, ds);


	if(offset < num_of_analog_datasets) {
		//ANALOG
		first = DATASET_MAX_SIZE*offset;
		last = DATASET_MAX_SIZE*(offset+1);
		for (i = first; (i<last) &&( i < num_of_analog_ids);i++){
			var = MmsVariableAccessSpecification_create (IDICCP, analog[i].id);
			var->arrayIndex = 0;	
			LinkedList_add(variables, var);
		}
		dataset_conf[offset].type = DATASET_ANALOG;
		dataset_conf[offset].size = i-first;
		dataset_conf[offset].offset = first;

	} else if(offset < (num_of_analog_datasets + num_of_digital_datasets)){ 
		//DIGITAL
		first = DATASET_MAX_SIZE*(offset-num_of_analog_datasets);
		last = DATASET_MAX_SIZE*(offset-num_of_analog_datasets +1);
		for (i = first; (i<last) &&( i < num_of_digital_ids);i++){
			var = MmsVariableAccessSpecification_create (IDICCP, digital[i].id);
			var->arrayIndex = 0;	
			LinkedList_add(variables, var);
		}
		dataset_conf[offset].type = DATASET_DIGITAL;
		dataset_conf[offset].size = i-first;
		dataset_conf[offset].offset = first;

	} else if(offset < num_of_datasets){ 
		//EVENTS
		first = DATASET_MAX_SIZE*(offset-num_of_analog_datasets-num_of_digital_datasets);
		last = DATASET_MAX_SIZE*(offset-num_of_analog_datasets-num_of_digital_datasets +1);
		for (i = first; (i<last) &&( i < num_of_event_ids);i++){
			var = MmsVariableAccessSpecification_create (IDICCP, events[i].id);
			var->arrayIndex = 0;	
			LinkedList_add(variables, var);
		}
		dataset_conf[offset].type = DATASET_EVENTS;
		dataset_conf[offset].size = i-first;
		dataset_conf[offset].offset = first;


	}else {
		printf("ERROR creating unknown dataset\n");
	}

	MmsConnection_defineNamedVariableList(con, &mmsError, IDICCP, ds_name, variables); 

	LinkedList_destroy(variables);
}

/*********************************************************************************************************/

static void
informationReportHandler (void* parameter, char* domainName, char* variableListName, MmsValue* value, LinkedList attributes, int attributesCount)
{
	int offset = 0;
	int i = 0;
	time_t time_stamp;
	char * domain_id = NULL;
	char * transfer_set = NULL;
	MmsError mmsError;
	int octet_offset = 0;
	int offset_size = 0;
	unsigned short time_stamp_extended =0;
	unsigned char * ts_extended;
	ts_extended = (char *)&time_stamp_extended;
	time(&time_stamp);
	if (value != NULL && attributes != NULL && attributesCount ==4 && parameter != NULL) {
		LinkedList list_names	 = LinkedList_getNext(attributes);
		while (list_names != NULL) {
			char * attribute_name = (char *) list_names->data;
			if(attribute_name == NULL){
				i++;
				fprintf (error_file, "ERROR - received report with null atribute name\n");
				fflush(error_file);
				continue;
			}
			list_names = LinkedList_getNext(list_names);
			MmsValue * dataSetValue = MmsValue_getElement(value, i);
			if(dataSetValue == NULL){
				i++;
				fprintf (error_file, "ERROR - received report with null dataset\n");
				fflush(error_file);
				continue;
			}

			if (strncmp("Transfer_Set_Name", attribute_name, 17) == 0) {
				MmsValue* ts_name;
				MmsValue* d_id;
				if(dataSetValue !=NULL) {
					d_id = MmsValue_getElement(dataSetValue, 1);
					if(d_id !=NULL) {
						domain_id = MmsValue_toString(d_id);
					} else {
						fprintf(error_file, "ERROR - Empty domain id on report\n");
						fflush(error_file);
					}
					ts_name = MmsValue_getElement(dataSetValue, 2);
					if(ts_name !=NULL) {
						transfer_set = MmsValue_toString(ts_name);
					} else {
						fprintf(error_file, "ERROR - Empty transfer set name on report\n");
						fflush(error_file);
					}
				} else {
					fprintf(error_file,"ERROR - Empty data for transfer set report\n");
					fflush(error_file);
				}					
				i++;
				continue;
			}


			if (strncmp("Transfer_Set_Time_Stamp", attribute_name, 23) == 0) {
				time_stamp =  MmsValue_toUint32(dataSetValue);
				i++;
				continue;
			}

			for (offset=0; offset<num_of_datasets;offset++) {

				if (strncmp(attribute_name,dataset_conf[offset].id,24) == 0) {

					if(dataSetValue != NULL){

						if (dataSetValue->value.octetString.buf != NULL) {
							unsigned int index = 0;	
							
							//DEBUG BUFFER
							  /* int j;
							   for (j=0; j < dataSetValue->value.octetString.size; j ++){
							   printf(" %x", dataSetValue->value.octetString.buf[j]);
							   }
							   printf("\n"); */

							//RULE 0
							//first byte is the rule
							if(dataSetValue->value.octetString.buf[0] == 0) {
								if(dataSetValue->value.octetString.size == 0) {
									fprintf(error_file, "ERROR - empty octetString\n");
									fflush(error_file);
									return;
								}else if(dataSetValue->value.octetString.size < 2) {
									fprintf(error_file, "ERROR - no index in the octed string\n");
									fflush(error_file);
									return;
								}

								octet_offset = 1; 
								index = dataset_conf[offset].offset; // config index

								while (octet_offset < dataSetValue->value.octetString.size){
									if(dataset_conf[offset].type == DATASET_DIGITAL){
										if(!digital[index].not_present){//In order to not threat non existent objects
											if( octet_offset+ RULE0_DIGITAL_REPORT_SIZE <= dataSetValue->value.octetString.size) {
												time_data time_value;
												time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
												time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
												time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
												time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
												ts_extended[1]= dataSetValue->value.octetString.buf[octet_offset+4];
												ts_extended[0]= dataSetValue->value.octetString.buf[octet_offset+5];
												handle_digital_integrity(dataSetValue->value.octetString.buf[octet_offset+6], index, time_value.t, time_stamp_extended);
											} else {
												fprintf(error_file, "ERROR - wrong digital report octet size %d, offset %d - data %d bytes - ds %d, idx %d \n",dataSetValue->value.octetString.size, octet_offset, RULE0_DIGITAL_REPORT_SIZE, offset, index );
												fflush(error_file);
											}
										}
										octet_offset = octet_offset + RULE0_DIGITAL_REPORT_SIZE;
									} else if(dataset_conf[offset].type == DATASET_EVENTS){
										if(!events[index].not_present){ //In order to not threat non existent objects
											if( octet_offset+RULE0_DIGITAL_REPORT_SIZE <= dataSetValue->value.octetString.size){
												time_data time_value;
												time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
												time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
												time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
												time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
												ts_extended[1]= dataSetValue->value.octetString.buf[octet_offset+4];
												ts_extended[0]= dataSetValue->value.octetString.buf[octet_offset+5];
												handle_event_integrity(dataSetValue->value.octetString.buf[octet_offset+6], index, time_value.t, time_stamp_extended);
											} else {
												fprintf(error_file, "ERROR - wrong event report octet size %d, offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE0_DIGITAL_REPORT_SIZE, offset, index);
												fflush(error_file);
											}
										}
										octet_offset = octet_offset + RULE0_DIGITAL_REPORT_SIZE;
									} else if(dataset_conf[offset].type == DATASET_ANALOG){
										offset_size=RULE0_ANALOG_REPORT_SIZE;
										if(analog[index].not_present){
											if(octet_offset+2 <= dataSetValue->value.octetString.size){ 
												if(dataSetValue->value.octetString.buf[octet_offset] == 0x53 && dataSetValue->value.octetString.buf[octet_offset+1] == 0xf3) {
													if(octet_offset+6 <= dataSetValue->value.octetString.size){ 
														if(dataSetValue->value.octetString.buf[octet_offset+6] == 0x71){
															offset_size=RULE0_DIGITAL_REPORT_SIZE;//TODO: for analog non existent objects the size is 7 and not threated when 0x53F3xxyy000071
														}
													}
												}
											}
										}
										else{ 
											if(octet_offset+offset_size <= dataSetValue->value.octetString.size) {
												float_data data_value;
												data_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
												data_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
												data_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
												data_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
												handle_analog_integrity(data_value.f, dataSetValue->value.octetString.buf[octet_offset+4], index, time_stamp);
											} else {
												fprintf(error_file, "ERROR - wrong analog report octet size %d, offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE0_ANALOG_REPORT_SIZE, offset, index);
												fflush(error_file);
											}
										}
										octet_offset = octet_offset + offset_size;
									}  else {
										fprintf(error_file, "ERROR - wrong index %d\n", offset);
										fflush(error_file);
										return;
									}
									index++;
								}
							}else if(dataSetValue->value.octetString.buf[0] == 1) {
								//RULE 1 - not implemented
								fprintf(error_file, "WARNING - Information Report with rule 1 received\n");
								fflush(error_file);
								return;
							}
							else if (dataSetValue->value.octetString.buf[0] == 2) {
								//RULE 2
								if(dataSetValue->value.octetString.size == 0) {
									fprintf(error_file,"ERROR - empty octetString\n");
									fflush(error_file);
									return;
								}

								octet_offset = 1; //first byte is the rule

								while (octet_offset < dataSetValue->value.octetString.size){
									// Packet INDEX
									index = (dataSetValue->value.octetString.buf[octet_offset]<<8 | dataSetValue->value.octetString.buf[octet_offset+1])-INDEX_OFFSET;
									// Translate into configuration index
									index = index + dataset_conf[offset].offset; // config index
									
									if(dataset_conf[offset].type == DATASET_DIGITAL){
										if((RULE2_DIGITAL_REPORT_SIZE+octet_offset) <= dataSetValue->value.octetString.size ) {
											time_data time_value;
											time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
											time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
											time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
											time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];
											ts_extended[1]= dataSetValue->value.octetString.buf[octet_offset+6];
											ts_extended[0]= dataSetValue->value.octetString.buf[octet_offset+7];
											//printf("%x %x\n", dataSetValue->value.octetString.buf[6],dataSetValue->value.octetString.buf[7]); //TODO Timestamp
											handle_digital_report(dataSetValue->value.octetString.buf[octet_offset+RULE2_DIGITAL_REPORT_SIZE-1], index, time_value.t, time_stamp_extended);
										}else {
											fprintf(error_file,"ERROR - Wrong size of digital report %d, octet_offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE2_DIGITAL_REPORT_SIZE, offset, index );
											fflush(error_file);
										}
										octet_offset = octet_offset + RULE2_DIGITAL_REPORT_SIZE;

									} else if(dataset_conf[offset].type == DATASET_EVENTS){
										if( (RULE2_DIGITAL_REPORT_SIZE+octet_offset) <= dataSetValue->value.octetString.size) {
											time_data time_value;
											time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
											time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
											time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
											time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];
											ts_extended[1]= dataSetValue->value.octetString.buf[octet_offset+6];
											ts_extended[0]= dataSetValue->value.octetString.buf[octet_offset+7];
											handle_event_report(dataSetValue->value.octetString.buf[octet_offset+RULE2_DIGITAL_REPORT_SIZE-1], index, time_value.t, time_stamp_extended);
										}else{
											fprintf(error_file,"ERROR - Wrong size of event report %d, octet_offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE2_DIGITAL_REPORT_SIZE, offset, index  );
											fflush(error_file);
										}
										octet_offset = octet_offset + RULE2_DIGITAL_REPORT_SIZE;
									} else if(dataset_conf[offset].type == DATASET_ANALOG){
										if((RULE2_ANALOG_REPORT_SIZE + octet_offset)<= dataSetValue->value.octetString.size ) {
											float_data data_value;
											data_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
											data_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
											data_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
											data_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];
											handle_analog_report(data_value.f, dataSetValue->value.octetString.buf[octet_offset+6], index, time_stamp);
										}else {
											fprintf(error_file,"ERROR - Wrong size of analog report %d, octet_offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE2_ANALOG_REPORT_SIZE, offset, index  );
											fflush(error_file);
										}
										octet_offset = octet_offset + RULE2_ANALOG_REPORT_SIZE;
									} else {
										fprintf(error_file,"ERROR - unkonwn information report index %d\n", index);
										fflush(error_file);
										/*int j;
										for (j=0; j < dataSetValue->value.octetString.size; j ++){
											printf(" %x", dataSetValue->value.octetString.buf[j]);
										}*/
										MmsValue_delete(value);
										//LinkedList_destroy(attributes);
										return;
									}
								}
							} else {
								fprintf(error_file,"ERROR - invalid RULE\n");
								fflush(error_file);
							}
							//after handling report flush files
#if DATA_LOG
							switch(dataset_conf[offset].type) {
								case DATASET_ANALOG:
								   fflush(data_file_analog);	
								   break;
								case DATASET_DIGITAL:
								   fflush(data_file_digital);
								   break;
								case DATASET_EVENTS:
								   fflush(data_file_events);
								   break;
							}
#endif
						} else {
							fprintf(error_file,"ERROR - empty bitstring\n");
							fflush(error_file);
						}
					} else {
						fprintf(error_file,"ERROR - NULL Element\n");
						fflush(error_file);
					}
				}
			}
			i++;
		}
		//CONFIRM RECEPTION OF EVENT
		MmsConnection_sendUnconfirmedPDU((MmsConnection)parameter,&mmsError,domain_id, transfer_set, time_stamp);
		MmsValue_delete(value);
		//LinkedList_destroy(attributes);
	} else{
		fprintf(error_file, "ERROR - wrong report %d %d %d %d\n",value != NULL, attributes != NULL , attributesCount , parameter != NULL);
		fflush(error_file);
	}
	//TODO: free values
}

/*********************************************************************************************************/

static int read_configuration() {
	FILE * file = NULL;
	FILE * log_file = NULL;
	char line[300];
	int origin = 0;
	int event = 0;
	unsigned int nponto = 0;
	unsigned int nponto_monitored = 0;
	char id_ponto[MAX_STR_NAME] = "";
	char state_name[MAX_STR_NAME] = "";
	int state_split = 0;
	int state_data=0;
	char type = 0;
	int utr_addr=0;
	int i;
	const char *str1;
	char id_iccp[MAX_STR_NAME], cfg_file[MAX_STR_NAME], cfg_log[MAX_STR_NAME], error_log[MAX_STR_NAME];
	char config_param[MAX_STR_NAME], config_value[MAX_STR_NAME];
	int cfg_params = 0;

	/*****************
	 * OPEN LOG FILES
	 * **********/

	// OPEN ERROR LOG
	time_t t = time(NULL);
	struct tm now = *localtime(&t); 
	snprintf(error_log,MAX_STR_NAME, "iccp_error-%04d%02d%02d%02d%02d.log", now.tm_year+1900, now.tm_mon+1, now.tm_mday,now.tm_hour, now.tm_min);
	error_file = fopen(error_log, "w");
	if(error_file==NULL){
		printf("Error, cannot open error log file %s\n",error_log);
		fclose(error_file);
		return -1;
	}

	snprintf(cfg_log,MAX_STR_NAME, "iccp_config-%04d%02d%02d%02d%02d.log", now.tm_year+1900, now.tm_mon+1, now.tm_mday,now.tm_hour, now.tm_min);
	log_file = fopen(cfg_log, "w");
	if(log_file==NULL){
		printf("Error, cannot open configuration log file %s\n", cfg_log);
		fclose(error_file);
		fclose(log_file);
		return -1;
	} 

	/*****************
	 * READ ICCP CONFIGURATION PARAMETERS
	 **********/

	file = fopen(ICCP_CLIENT_CONFIG_FILE, "r");
	if(file==NULL){
		fprintf(error_file, "ERROR -  cannot open configuration file %s\n", ICCP_CLIENT_CONFIG_FILE);
		fclose(log_file);
		return -1;
	} else{
		while ( fgets(line, 300, file)){
			if (line[0] == '/' && line[1]=='/')
				continue;
			if(sscanf(line, "%[^=]=\"%[^\";]; ",config_param, config_value) < 1)
				break;
			if(strcmp(config_param, "IDICCP") == 0){
				snprintf(IDICCP, MAX_ID_ICCP_NAME, "%s", config_value);
				fprintf(log_file,"IDICCP=%s\n", IDICCP);
				cfg_params++;
			}
			if(strcmp(config_param, "SERVER_NAME_1") == 0){
				snprintf(srv1, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"SERVER_NAME_1=%s\n", srv1);
				cfg_params++;
			}
			if(strcmp(config_param, "SERVER_NAME_2") == 0){
				snprintf(srv2, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"SERVER_NAME_2=%s\n", srv2);
				cfg_params++;
			}
			if(strcmp(config_param, "SERVER_NAME_3") == 0){
				snprintf(srv3, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"SERVER_NAME_3=%s\n", srv3);
				cfg_params++;
			}
			if(strcmp(config_param, "SERVER_NAME_4") == 0){
				snprintf(srv4, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"SERVER_NAME_4=%s\n", srv4);
				cfg_params++;
			}
			if(strcmp(config_param, "SERVER_NAME_5") == 0){
				snprintf(srv5, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"SERVER_NAME_5=%s\n", srv5);
				cfg_params++;
			}
			if(strcmp(config_param, "SERVER_NAME_6") == 0){
				snprintf(srv6, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"SERVER_NAME_6=%s\n", srv6);
				cfg_params++;
			}
			if(strcmp(config_param, "SERVER_NAME_7") == 0){
				snprintf(srv7, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"SERVER_NAME_7=%s\n", srv7);
				cfg_params++;
			}
			if(strcmp(config_param, "SERVER_NAME_8") == 0){
				snprintf(srv8, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"SERVER_NAME_8=%s\n", srv8);
				cfg_params++;
			}

			if(strcmp(config_param, "IHM_ADDRESS") == 0){
				snprintf(ihm_addr, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"IHM_ADDRESS=%s\n", ihm_addr);
				cfg_params++;
			}
			if(strcmp(config_param, "CONFIG_FILE") == 0){
				snprintf(cfg_file, MAX_STR_NAME, "%s", config_value);
				fprintf(log_file,"CONFIG_FILE=%s\n", cfg_file);
				cfg_params++;
			}
			if(strcmp(config_param, "DATASET_ANALOG_INTEGRITY_TIME") == 0){
				analog_gi = atoi(config_value);
				fprintf(log_file,"DATASET_ANALOG_INTEGRITY_TIME=%d\n", analog_gi);
				cfg_params++;
			}
			if(strcmp(config_param, "DATASET_DIGITAL_INTEGRITY_TIME") == 0){
				digital_gi = atoi(config_value);
				fprintf(log_file,"DATASET_DIGITAL_INTEGRITY_TIME=%d\n", digital_gi);
				cfg_params++;
			}
			if(strcmp(config_param, "DATASET_EVENTS_INTEGRITY_TIME") == 0){
				events_gi = atoi(config_value);
				fprintf(log_file,"DATASET_EVENTS_INTEGRITY_TIME=%d\n", events_gi);
				cfg_params++;
			}
			if(strcmp(config_param, "DATASET_ANALOG_BUFFER_INTERVAL") == 0){
				analog_buf = atoi(config_value);
				fprintf(log_file,"DATASET_ANALOG_BUFFER_INTERVAL=%d\n", analog_buf);
				cfg_params++;
			}
			if(strcmp(config_param, "DATASET_DIGITAL_BUFFER_INTERVAL") == 0){
				digital_buf = atoi(config_value);
				fprintf(log_file,"DATASET_DIGITAL_BUFFER_INTERVAL=%d\n", digital_buf);
				cfg_params++;
			}
			if(strcmp(config_param, "DATASET_EVENTS_BUFFER_INTERVAL") == 0){
				events_buf = atoi(config_value);
				fprintf(log_file,"DATASET_EVENTS_BUFFER_INTERVAL=%d\n", events_buf);
				cfg_params++;
			}
		}
	}

	if (cfg_params!=17){
		fprintf(error_file, "ERROR - wrong number of parameters on %s\n",ICCP_CLIENT_CONFIG_FILE);
		fclose(log_file);
		return -1;
	}

	/*****************
	 * START CONFIGURATIONS
	 **********/

    analog = calloc(DATASET_MAX_SIZE*DATASET_ANALOG_MAX_NUMBER, sizeof(data_config) ); 
    digital = calloc(DATASET_MAX_SIZE*DATASET_DIGITAL_MAX_NUMBER, sizeof(data_config) ); 
    events  = calloc(DATASET_MAX_SIZE*DATASET_EVENTS_MAX_NUMBER, sizeof(data_config) ); 
    commands  = calloc(COMMANDS_MAX_NUMBER , sizeof(command_config) ); 

	file = fopen(cfg_file, "r");
	if(file==NULL){
		fprintf(error_file, "ERROR - cannot open configuration file %s\n", cfg_file);
		return -1;
	} else{
		//first two rows of CONFIG_FILE are the reader, discard them
		if(!fgets(line, 300, file)){
			fprintf(error_file, "ERROR - Error reading %s file header\n", cfg_file);
			fclose(log_file);
			return -1;
		}else {
			if(sscanf(line, "%*s %*d %*s %d", &ihm_station) <1){
				fprintf(error_file, "ERROR - cannot get ihm station from header\n");
				fclose(log_file);
				return -1;
			}else {
				fprintf(log_file,"IHM station %d\n", ihm_station);
			}
		}

		if(!fgets(line, 300, file)){
			fprintf(error_file, "ERROR - Error reading %s file header second line\n", cfg_file);
			fclose(log_file);
			return -1;
		}

		while ( fgets(line, 300, file)){
			//if(sscanf(line, "%d %*d %22s %c", &configuration[num_of_ids].nponto,  configuration[num_of_ids].id, &configuration[num_of_ids].type ) <1)
			if(sscanf(line, "%d %*d %22s %c %31s %*d %*d %*d %d %*c %d %*d %*f %*f %d %d", &nponto,  id_ponto, &type, state_name, &origin, &utr_addr, &nponto_monitored, &event ) <1)
				break;

			//change - for $
			for ( i=0; i <22; i++) {
				if (id_ponto[i] == '-' || id_ponto[i] == '+'){
					id_ponto[i] = '$';
				}
			}
	
			//Command Digital or Analog
			if((type == 'D'||type=='A') && origin ==7 ){
				//add $C to the end of command
				if (id_ponto[21] == 'K') {
					id_ponto[22] = '$';
					id_ponto[23] = 'C';
				}
				memcpy(commands[num_of_commands].id,id_ponto,25);
				commands[num_of_commands].nponto = nponto;
				commands[num_of_commands].monitored = nponto_monitored;
				if(type=='D')
					commands[num_of_commands].type = DATASET_COMMAND_DIGITAL;
				else
					commands[num_of_commands].type = DATASET_COMMAND_ANALOG;
				num_of_commands++;
			}//Events
			else if(type == 'D' && event == 3){
				memcpy(events[num_of_event_ids].id,id_ponto,25);
				events[num_of_event_ids].nponto = nponto;
				events[num_of_event_ids].utr_addr = utr_addr;
				
				state_split=0;
				for ( i=0; i <MAX_STR_NAME; i++) {
					if (state_name[i] == '/' ){
						state_split=i;
						events[num_of_event_ids].state_on[i]=0;
						continue;
					}
					if(state_split){
						if (state_name[i] == 0 ) {
							events[num_of_event_ids].state_off[i-state_split-1]=0;
							break;
						}else
							events[num_of_event_ids].state_off[i-state_split-1] = state_name[i];

					}else
						events[num_of_event_ids].state_on[i]=state_name[i];
				}
				num_of_event_ids++;
			} //Digital
			else if(type == 'D'){
				memcpy(digital[num_of_digital_ids].id,id_ponto,25);
				digital[num_of_digital_ids].nponto = nponto;
				digital[num_of_digital_ids].utr_addr = utr_addr;
				
				state_split=0;
				for ( i=0; i <MAX_STR_NAME; i++) {
					if (state_name[i] == '/' ){
						state_split=i;
						digital[num_of_digital_ids].state_on[i]=0;
						continue;
					}
					if(state_split){
						if (state_name[i] == 0 ) {
							digital[num_of_digital_ids].state_off[i-state_split-1] =0;
							break;
						}else
							digital[num_of_digital_ids].state_off[i-state_split-1] = state_name[i];
					}else
						digital[num_of_digital_ids].state_on[i]=state_name[i];
				}
				num_of_digital_ids++;
			} //Analog
			else if(type == 'A'){
				memcpy(analog[num_of_analog_ids].id,id_ponto,25);
				analog[num_of_analog_ids].nponto = nponto;
				analog[num_of_analog_ids].utr_addr = utr_addr;
				memcpy(analog[num_of_analog_ids].state_on,state_name,  16);
				num_of_analog_ids++;
			} //Unknown 
			else {
				fprintf(error_file,"WARNING - ERROR reading configuration file! Unknown type");
				fflush(error_file);
			}
		}
	}

	fprintf(log_file, "***************ANALOG***********************\n");
	for (i=0; i < num_of_analog_ids; i++) {
		fprintf(log_file, "%d %d \t%s\t \n",i, analog[i].nponto,  analog[i].id);
	}

	fprintf(log_file, "***************DIGITAL***********************\n");
	for (i=0; i < num_of_digital_ids; i++) {
		fprintf(log_file, "%d %d \t%s\t \n",i, digital[i].nponto,  digital[i].id);
	}

	fprintf(log_file, "***************EVENTS***********************\n");
	for (i=0; i < num_of_event_ids; i++) {
		fprintf(log_file, "%d %d \t%s\t \n",i, events[i].nponto,  events[i].id);
	}

	fprintf(log_file, "***************COMMANDS***********************\n");
	for (i=0; i < num_of_commands; i++) {
		fprintf(log_file, "%d %d \t%s\t %d \t\n",i, commands[i].nponto,  commands[i].id, commands[i].monitored);
	}

	num_of_analog_datasets = num_of_analog_ids/DATASET_MAX_SIZE;
	if(num_of_analog_ids%DATASET_MAX_SIZE)
		num_of_analog_datasets++;

	num_of_digital_datasets += num_of_digital_ids/DATASET_MAX_SIZE;
	if(num_of_digital_ids%DATASET_MAX_SIZE)
		num_of_digital_datasets++;

	num_of_event_datasets += num_of_event_ids/DATASET_MAX_SIZE;
	if(num_of_event_ids%DATASET_MAX_SIZE)
		num_of_event_datasets++;

	num_of_datasets = num_of_analog_datasets + num_of_digital_datasets + num_of_event_datasets;

	//alloc data for datasets
	dataset_conf = calloc(num_of_datasets, sizeof(dataset_config));

	for (i=0; i < num_of_datasets; i++) {
		snprintf(dataset_conf[i].id, DATASET_NAME_SIZE, "ds_%03d", i);
	}

	fclose(file);
	fclose(log_file);
	return 0;
}

/*********************************************************************************************************/

static void sigint_handler(int signalId)
{
	running = 0;
}

/*********************************************************************************************************/

static void cleanup_variables(MmsConnection con)
{
	printf("cleanning up variables\n");

#ifdef DATA_LOG
	fclose(data_file_analog);
	fclose(data_file_digital);
	fclose(data_file_events);
#endif
	fclose(error_file);
	free(dataset_conf);
	free(analog);
	free(digital);
	free(events);
	MmsConnection_destroy(con);
	if(ihm_enabled){
		close(ihm_socket_send);
		close(ihm_socket_receive);
	}
}

/*********************************************************************************************************/

#ifdef DATA_LOG
static int open_data_logs(void) {
	data_file_analog = fopen(DATA_ANALOG_LOG, "w");
	if(data_file_analog==NULL){
		printf("Error, cannot open configuration data log file %s\n", DATA_ANALOG_LOG);
		return -1;
	}
	data_file_digital = fopen(DATA_DIGITAL_LOG, "w");
	if(data_file_digital==NULL){
		printf("Error, cannot open configuration data log file %s\n", DATA_DIGITAL_LOG);
		return -1;
	}
	data_file_events = fopen(DATA_EVENTS_LOG, "w");
	if(data_file_events==NULL){
		printf("Error, cannot open configuration data log file %s\n", DATA_EVENTS_LOG);
		return -1;
	}
	return 0;

}
#endif

/*********************************************************************************************************/

//(TODO: add connection handler)
static int connect_to_iccp_server(MmsConnection * con){
	if ((connect_to_server(*con, srv1) < 0)){
		MmsConnection_destroy(*con);
		*con = MmsConnection_create();
		if ((connect_to_server(*con, srv2) < 0)){
			MmsConnection_destroy(*con);
			*con = MmsConnection_create();
			if ((connect_to_server(*con, srv3) < 0)){
				MmsConnection_destroy(*con);
				*con = MmsConnection_create();
				if ((connect_to_server(*con, srv4) < 0)){
					MmsConnection_destroy(*con);
					*con = MmsConnection_create();
					if ((connect_to_server(*con, srv5) < 0)){
						MmsConnection_destroy(*con);
						*con = MmsConnection_create();
						if ((connect_to_server(*con, srv6) < 0)){
							MmsConnection_destroy(*con);
							*con = MmsConnection_create();
							if ((connect_to_server(*con, srv7) < 0)){
								MmsConnection_destroy(*con);
								*con = MmsConnection_create();
								if ((connect_to_server(*con, srv8) < 0)){
									return -1;
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

/*********************************************************************************************************/

static int create_ihm_comm(){
	ihm_socket_send = prepare_Send(ihm_addr, PORT_IHM_TRANSMIT, &ihm_sock_addr);
	if(ihm_socket_send < 0){
		printf("could not create UDP socket to trasmit to IHM\n");
		return -1;
	}	
	printf("Created UDP socket for IHM %s Port %d\n",ihm_addr, PORT_IHM_TRANSMIT);

	ihm_socket_receive = prepare_Wait(ihm_addr, PORT_IHM_LISTEN);
	if(ihm_socket_receive < 0){
		printf("could not create UDP socket to listen to IHM\n");
		close(ihm_socket_send);
		return -1;
	}
	printf("Created UDP local socket for IHM Port %d\n",PORT_IHM_LISTEN);
	return 0;
}

/*********************************************************************************************************/
static void check_commands(MmsConnection con){
	char * msg_rcv;
	msg_rcv = WaitT(ihm_socket_receive, 2000);	
	if(msg_rcv != NULL) {
		int i;
		t_msgcmd cmd_recv = {0};
		memcpy(&cmd_recv, msg_rcv, sizeof(t_msgcmd));
		if(cmd_recv.signature != 0x4b4b4b4b) {
			char * cmd_debug = (char *)&cmd_recv;
			fprintf(error_file,"ERROR - command received with wrong signature!\n");  
			for (i=0; i < sizeof(t_msgcmd); i++){
				fprintf(error_file," %02x", cmd_debug[i]);
			}
			fprintf(error_file,"\n");
			fflush(error_file);
		}
		for (i=0; i < num_of_commands; i++) {
			if(cmd_recv.endereco==commands[i].nponto)
				break;
		}
		if(i<num_of_commands){
			fprintf(error_file, "Command %d, type %d, onoff %d, sbo %d, qu %d, utr %d\n", cmd_recv.endereco, cmd_recv.tipo, 
					cmd_recv.onoff, cmd_recv.sbo, cmd_recv.qu, cmd_recv.utr);
			printf("Command %d, type %d, onoff %d, sbo %d, qu %d, utr %d\n", cmd_recv.endereco, cmd_recv.tipo, 
					cmd_recv.onoff, cmd_recv.sbo, cmd_recv.qu, cmd_recv.utr);
			if(command_variable(con, error_file, commands[i].id, cmd_recv.onoff)<0){
				fprintf(error_file,"Error writing %d to %s\n", cmd_recv.onoff, commands[i].id);
				fflush(error_file);
			} else {
				int j;
				fprintf(error_file,"Done writing %d to %s\n", cmd_recv.onoff, commands[i].id);
				fflush(error_file);
				printf("Done writing %d to %s variable\n", cmd_recv.onoff, commands[i].id);
				if (commands[i].type== DATASET_COMMAND_DIGITAL){
					for(j=0; j<num_of_digital_ids; j++){
						if(digital[j].nponto == commands[i].monitored){
							printf("send digital cmd response\n");
							send_digital_to_ihm(ihm_socket_send, &ihm_sock_addr, digital[j].nponto, digital[j].utr_addr, ihm_station, digital[j].state, digital[j].time_stamp, digital[j].time_stamp_extended, 0, 1);
							break;
						}
					}
				}else if (commands[i].type== DATASET_COMMAND_ANALOG){
					for(j=0; j<num_of_analog_ids; j++){
						if(analog[j].nponto == commands[i].monitored){
							printf("send anlog cmd response\n");
							send_analog_to_ihm(ihm_socket_send, &ihm_sock_addr, analog[j].nponto, analog[j].utr_addr,ihm_station, analog[j].f, analog[j].state, 0, 1);
							break;
						}
					}
				}else{
					fprintf(error_file,"Wrong command type %d for %s \n", commands[i].type, commands[i].id);
					fflush(error_file);
				}
			}
		}else{
			char * cmd_debug = (char *)&cmd_recv;
			fprintf(error_file,"ERROR - command received %d not found! \n", cmd_recv.endereco);  
			for (i=0; i < sizeof(t_msgcmd); i++){
				fprintf(error_file," %02x", cmd_debug[i]);
			}
			fprintf(error_file,"\n");
			fflush(error_file);
		}
	}
}

/*********************************************************************************************************/

int main (int argc, char ** argv){
	unsigned int i = 0;
	MmsError mmsError;
	signal(SIGINT, sigint_handler);
	MmsConnection con = MmsConnection_create();
		
	// READ CONFIGURATION FILE
	if (read_configuration() != 0) {
		printf("Error reading configuration\n");
		return -1;
	} else {
		printf("Start configuration with %d analog, %d digital, %d events, %d commands, and %d datasets\n", num_of_analog_ids, num_of_digital_ids, num_of_event_ids, num_of_commands, num_of_datasets);
	}

	// OPEN DATA LOG FILES	
#ifdef DATA_LOG
	if(open_data_logs()<0) {
		printf("Error, cannot open configuration data log files\n");
		cleanup_variables(con);
		return -1;
	}
#endif

	//INITIALIZE IHM CONNECTION 
	if(strcmp(ihm_addr,"no")==0) {
		printf("no ihm configured\n");
	}else{
		ihm_enabled=1;
		if(create_ihm_comm() < 0){
			printf("Error, cannot open communication to ihm server\n");
			cleanup_variables(con);
			return -1;
		}
	}
	/*while(running){
		SendT(ihm_socket,(char *)srv1, 10, &ihm_sock_addr);
		Thread_sleep(2000);
	}
	cleanup_variables(con);
	return -1;
*/
	//INITIALIZE ICCP CONNECTION 
	if(connect_to_iccp_server(&con) < 0){
		printf("Error, cannot connect to iccp server\n");
		cleanup_variables(con);
		return -1;
	}


	// DELETE DATASETS WHICH WILL BE USED
	printf("deleting dada sets     ");
	for (i=0; i < num_of_datasets; i++) {
		fflush(stdout);
		MmsConnection_deleteNamedVariableList(con,&mmsError, IDICCP, dataset_conf[i].id);
		printf(".");
	}
	printf("\n");

	// CREATE TRASNFERSETS ON REMOTE SERVER	
	printf("creating transfer sets ");
	for (i=0; i < num_of_datasets; i++){
		fflush(stdout);
		MmsValue *transfer_set_dig  = get_next_transferset(con,IDICCP,  error_file);
		if( transfer_set_dig == NULL) {	
			printf("\nCould not create transfer set for digital data\n");
			cleanup_variables(con);
			return -1;
		} else {
			strncpy(dataset_conf[i].ts, MmsValue_toString(transfer_set_dig), TRANSFERSET_NAME_SIZE);
			MmsValue_delete(transfer_set_dig);
		}
		Thread_sleep(50);//sleep 50ms for different report times (better handling)
		printf(".");
	}	
	printf("\n");

	// CREATE DATASETS WITH CUSTOM VARIABLES
	printf("creating data sets     ");
	for (i=0; i < num_of_datasets; i++){
		fflush(stdout);
		create_dataset(con, dataset_conf[i].id,i);
		printf(".");
	}
	printf("\n");

	// WRITE DATASETS TO TRANSFERSET
	printf("writing data sets      ");
	for (i=0; i < num_of_datasets; i++){
		fflush(stdout);
		if(dataset_conf[i].type == DATASET_ANALOG) 
			write_dataset(con, IDICCP, dataset_conf[i].id, dataset_conf[i].ts, analog_buf, analog_gi, 0);
		else if(dataset_conf[i].type == DATASET_DIGITAL)
			write_dataset(con, IDICCP, dataset_conf[i].id, dataset_conf[i].ts, digital_buf, digital_gi, 1);
		else if(dataset_conf[i].type == DATASET_EVENTS)
			write_dataset(con, IDICCP, dataset_conf[i].id, dataset_conf[i].ts, events_buf, events_gi, 1);
		else{
			printf("\nunknown write dataset type\n");
			cleanup_variables(con);
			return -1;
		}
		Thread_sleep(50);//sleep 50ms for different report times (better handling)
		printf(".");
	}
	printf("\n");
	
	// READ VARIABLES
	printf("Reading data sets      ");
	for (i=0; i < num_of_datasets; i++){
		fflush(stdout);
		read_dataset(con, dataset_conf[i].id, i);
		Thread_sleep(50);//sleep 50ms in order to space handling for udp packets on ihm interface(better handling)
		printf(".");
	}
	printf("\n");

	// HANDLE REPORTS
	MmsConnection_setInformationReportHandler(con, informationReportHandler, (void *) con);
	printf("ICCP Process Successfully Started!\n");
	
	// LOOP TO MANTAIN CONNECTION ACTIVE AND CHECK COMMANDS	
	while(running) {
		if(ihm_enabled)
			check_commands(con);
		
		if (check_connection(con,IDICCP, error_file) < 0)
			break;
	}

	cleanup_variables(con);

	return 0;
}
