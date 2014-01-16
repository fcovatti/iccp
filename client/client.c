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
#include "util.h"
#include "config.h"

//#define CLIENT_DEBUG 1
#define DATA_LOG 1


static int running = 1;

//Configuration
static int num_of_analog_ids = 0;	
static int num_of_digital_ids = 0;	
static int num_of_event_ids = 0;	

static int num_of_datasets = 0;
static int num_of_analog_datasets = 0;
static int num_of_digital_datasets = 0;
static int num_of_event_datasets = 0;

static data_config * analog = NULL;
static data_config * digital = NULL;
static data_config * events = NULL;

static dataset_config * dataset_conf = NULL;

#ifdef DATA_LOG
static FILE * data_file_analog = NULL;
static FILE * data_file_digital = NULL;
static FILE * data_file_events = NULL;
#endif

static FILE * error_file = NULL;

static int handle_analog_state(float value, unsigned char state, int index,time_t time_stamp);
static int handle_digital_state(unsigned char state, int index, time_t time_stamp);
static int handle_event_state(unsigned char state, int index, time_t time_stamp);

static int handle_analog_report(float value, unsigned char state, int index,time_t time_stamp){
	return handle_analog_state(value,state,index,time_stamp);
}

static int handle_digital_report(unsigned char state, int index,time_t time_stamp){
	return handle_digital_state(state, index, time_stamp);
}

static int handle_event_report(unsigned char state, int index,time_t time_stamp){
	return handle_event_state(state, index, time_stamp);
}

static int handle_analog_integrity(float value, unsigned char state, int index,time_t time_stamp){
	return handle_analog_state(value,state,index,time_stamp);
}
static int handle_digital_integrity(unsigned char state, int index,time_t time_stamp){
	return handle_digital_state(state, index, time_stamp);
}

static int handle_event_integrity(unsigned char state, int index,time_t time_stamp){
	return handle_event_state(state, index, time_stamp);
}

static int handle_analog_state(float value, unsigned char state, int index,time_t time_stamp){

	if (analog != NULL && index < num_of_analog_ids) {

#ifdef DATA_LOG
		if(	(analog[index].f != value) || (analog[index].state != state)){
			//fprintf(data_file_analog, "%08d %02X %u %.2f\n", analog[index].nponto, state, (int)time_stamp, value);
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
#ifdef CLIENT_DEBUG	
		printf("%25s: %06.07f |", analog[index].id, value);
		print_value(state,1, time_stamp);
#endif
	}
	return 0;
}

static int handle_digital_state(unsigned char state, int index, time_t time_stamp){

	if (digital != NULL && index < num_of_digital_ids) {

#ifdef DATA_LOG
		if((digital[index].state != state)){
			//fprintf(data_file_digital, "%08d %02X %u\n", digital[index].nponto, state, (int)time_stamp);
			data_digital_out data;
			data.nponto = digital[index].nponto;
			data.state = state;
			data.time_stamp = time_stamp;
			fwrite(&data,1,sizeof(data_digital_out),data_file_digital);
		}
#endif
		digital[index].state = state;
		digital[index].time_stamp = time_stamp;
#ifdef CLIENT_DEBUG	
		printf("%25s: ", digital[index].id);
		print_value(state,0, time_stamp);
#endif
	}
	return 0;
}

static int handle_event_state(unsigned char state, int index, time_t time_stamp){

	if (events != NULL && index < num_of_event_ids) {

#ifdef DATA_LOG
		if((events[index].state != state)){
			//fprintf(data_file_events, "%08d %02X %u\n", events[index].nponto, state, (int)time_stamp);
			data_digital_out data;
			data.nponto = events[index].nponto;
			data.state = state;
			data.time_stamp = time_stamp;
			fwrite(&data,1,sizeof(data_digital_out),data_file_events);
		}
#endif
		events[index].state = state;
		events[index].time_stamp = time_stamp;
#ifdef CLIENT_DEBUG	
		printf("%25s: ", events[index].id);
		print_value(state,0, time_stamp);
#endif
	}
	return 0;
}

static int read_dataset(MmsConnection con, char * ds_name, int offset){
	MmsValue* dataSet;
	MmsError mmsError;
	int i, idx;

	MmsValue* dataSetValue;
	MmsValue* dataSetElem;
	MmsValue* timeStamp;
	time_t time_stamp;

	int number_of_variables = dataset_conf[offset].size; 

	dataSet = MmsConnection_readNamedVariableListValues(con, &mmsError, IDICCP, ds_name, 0);
	if (dataSet == NULL){
		fprintf(error_file,"ERROR - reading dataset failed! %d\n", mmsError);                                                                                                   
		return -1;
	}else{
		for (i=0; i < number_of_variables; i ++) {
			idx = i+dataset_conf[offset].offset;

			//GET DATASET VALUES (FIRST 3 VALUES ARE ACCESS-DENIED)
			dataSetValue = MmsValue_getElement(dataSet, INDEX_OFFSET+i);
			if(dataSetValue == NULL) {
				fprintf(error_file, "ERROR - could not get DATASET values offset %d element %d %d \n",offset, idx, number_of_variables);
				//TODO return -1;
				continue;
			}

			if(dataset_conf[offset].type == DATASET_ANALOG){
				MmsValue* analog_value;
				float analog_data;
				char analog_state;

				//First element Floating Point Value
				analog_value = MmsValue_getElement(dataSetValue, 0);
				if(analog_value == NULL) {
					fprintf(error_file, "ERROR - could not get floating point value %s\n", analog[idx].id);
					return -1;
				} 

				analog_data = MmsValue_toFloat(analog_value);

				//Second Element BitString data state
				analog_value = MmsValue_getElement(dataSetValue, 1);
				if(analog_value == NULL) {
					fprintf(error_file, "ERROR - could not get analog state %s\n", analog[idx].id);
					return -1;
				} 
				
				analog_state = analog_value->value.bitString.buf[0];
				time( &time_stamp);

				handle_analog_integrity(analog_data, analog_state,idx , time_stamp);
			} else if(dataset_conf[offset].type == DATASET_DIGITAL){
				//First element Data_TimeStampExtended
				dataSetElem = MmsValue_getElement(dataSetValue, 0);
				if(dataSetElem == NULL) {
					fprintf(error_file, "ERROR - could not get digital Data_TimeStampExtended %s\n", digital[idx].id);
					return -1;
				}
				timeStamp = MmsValue_getElement(dataSetElem, 0);

				if(timeStamp == NULL) {
					fprintf(error_file, "ERROR - could not get digital timestamp value %s\n", digital[idx].id);
					return -1;
				}
				time_stamp = MmsValue_toUint32(timeStamp);
				
				//Second Element DataState
				dataSetElem = MmsValue_getElement(dataSetValue, 1);
				if(dataSetElem == NULL) {
					fprintf(error_file, "ERROR - could not get digital DataState %s\n", digital[idx].id);
					return -1;
				}
				handle_digital_integrity(dataSetElem->value.bitString.buf[0], idx, time_stamp);


			} else if(dataset_conf[offset].type == DATASET_EVENTS){
				//First element Data_TimeStampExtended
				dataSetElem = MmsValue_getElement(dataSetValue, 0);
				if(dataSetElem == NULL) {
					fprintf(error_file, "ERROR - could not get event Data_TimeStampExtended %s\n", events[idx].id);
					return -1;
				}
				timeStamp = MmsValue_getElement(dataSetElem, 0);

				if(timeStamp == NULL) {
					fprintf(error_file, "ERROR - could not get event timestamp value %s\n", events[idx].id);
					return -1;
				}
				time_stamp = MmsValue_toUint32(timeStamp);
				
				//Second Element DataState
				dataSetElem = MmsValue_getElement(dataSetValue, 1);
				if(dataSetElem == NULL) {
					fprintf(error_file, "ERROR - could not get event DataState %s\n", events[idx].id);
					return -1;
				}
				handle_event_integrity(dataSetElem->value.bitString.buf[0], idx, time_stamp);
		
			} else if(dataset_conf[offset].type == DATASET_COMMANDS){
			/*	//First element Data_TimeStampExtended
				dataSetElem = MmsValue_getElement(dataSetValue, 0);
				if(dataSetElem == NULL) {
					fprintf(error_file, "ERROR - could not get digital Data_TimeStampExtended %s\n", configuration[i].id);
					return -1;
				}
				printf(" command MmsType %d\n",dataSetElem->type); 

				//Second Element DataState
				dataSetElem = MmsValue_getElement(dataSetValue, 1);
				if(dataSetElem == NULL) {
					fprintf(error_file, "ERROR - could not get command State %s\n", configuration[i].id);
					return -1;
				}

				printf("MmsType %d\n",dataSetElem->type); 
				*/
				printf("command dataset\n");
			} else{
				fprintf(error_file, "ERROR - unknown configuration type for dataset %d offset %d\n", offset, idx);
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

static void create_dataset(MmsConnection con, char * ds_name, int offset)
{
	MmsError mmsError;
	int i=0;
	int first = 0;
	int last = 0;
	LinkedList variables = LinkedList_create();

	//printf("create_data_Set %d\n", offset);
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
	time(&time_stamp);
	printf("*************Information Report Received %d********************\n", attributesCount);
	if (value != NULL && attributes != NULL && attributesCount ==4 && parameter != NULL) {
		LinkedList list_names	 = LinkedList_getNext(attributes);
		while (list_names != NULL) {
			char * attribute_name = (char *) list_names->data;
			if(attribute_name == NULL){
				i++;
				fprintf (error_file, "ERROR - received report with null atribute name\n");
				continue;
			}
			list_names = LinkedList_getNext(list_names);
			MmsValue * dataSetValue = MmsValue_getElement(value, i);
			if(dataSetValue == NULL){
				i++;
				fprintf (error_file, "ERROR - received report with null dataset\n");
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
					}
					ts_name = MmsValue_getElement(dataSetValue, 2);
					if(ts_name !=NULL) {
						transfer_set = MmsValue_toString(ts_name);
					} else {
						fprintf(error_file, "ERROR - Empty transfer set name on report\n");
					}
				} else {
					fprintf(error_file,"ERROR - Empty data for transfer set report\n");
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
							
							//DEBUG
							  /* int j;
							   for (j=0; j < dataSetValue->value.octetString.size; j ++){
							   printf(" %x", dataSetValue->value.octetString.buf[j]);
							   }
							   printf("\n"); */
						
							if(dataSetValue->value.octetString.buf[0] == 0) {


								if(dataSetValue->value.octetString.size == 0) {
									fprintf(error_file, "ERROR - empty octetString\n");
									return;
								}


								octet_offset = 1; //first byte is the rule
								index = dataset_conf[offset].offset; // config index

								while (octet_offset < dataSetValue->value.octetString.size){
									if(dataset_conf[offset].type == DATASET_DIGITAL){
										time_data time_value;
										time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
										time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
										time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
										time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
										handle_digital_integrity(dataSetValue->value.octetString.buf[octet_offset+6], index, time_value.t);
										octet_offset = octet_offset + 7;
									} else if(dataset_conf[offset].type == DATASET_EVENTS){
										time_data time_value;
										time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
										time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
										time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
										time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
										handle_event_integrity(dataSetValue->value.octetString.buf[octet_offset+6], index, time_value.t);
										octet_offset = octet_offset + 7;
									} else if(dataset_conf[offset].type == DATASET_ANALOG){
										float_data data_value;
										data_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
										data_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
										data_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
										data_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
										handle_analog_integrity(data_value.f, dataSetValue->value.octetString.buf[octet_offset+4], index, time_stamp);
										octet_offset = octet_offset + 5;
									} else if(dataset_conf[offset].type == DATASET_COMMANDS){
										printf("command no report\n");
									} else {
										fprintf(error_file, "ERROR - wrong index %d\n", offset);
										return;
									}
									index++;
								}


								//RULE 0
								//TODO: implement rule 0 handling
							}else if(dataSetValue->value.octetString.buf[0] == 1) {
								//RULE 1 - not implemented
								printf("Error - Information Report with rule 1 received\n");
								return;
							}
							else if (dataSetValue->value.octetString.buf[0] == 2) {
								//RULE 2
								if(dataSetValue->value.octetString.size == 0) {
									fprintf(error_file,"ERROR - empty octetString\n");
									return;
								}

								octet_offset = 1; //first byte is the rule

								while (octet_offset < dataSetValue->value.octetString.size){
									// Packet INDEX
									index = dataSetValue->value.octetString.buf[octet_offset]<<8 | (dataSetValue->value.octetString.buf[octet_offset+1]-INDEX_OFFSET);
									// Translate into configuration index
									index = index + dataset_conf[offset].offset; // config index
									
									if(dataset_conf[offset].type == DATASET_DIGITAL){
										if(dataSetValue->value.octetString.size < (RULE2_DIGITAL_REPORT_SIZE+octet_offset)) {
											fprintf(error_file,"ERROR - Wrong size of digital report %d, octet_offset %d\n",dataSetValue->value.octetString.size, octet_offset );
											MmsValue_delete(value);
											//LinkedList_destroy(attributes);
											return;
										}
										time_data time_value;
										time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
										time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
										time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
										time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];
										handle_digital_report(dataSetValue->value.octetString.buf[octet_offset+RULE2_DIGITAL_REPORT_SIZE-1], index, time_value.t);
										octet_offset = octet_offset + RULE2_DIGITAL_REPORT_SIZE;

									} else if(dataset_conf[offset].type == DATASET_EVENTS){
										if(dataSetValue->value.octetString.size < (RULE2_DIGITAL_REPORT_SIZE+octet_offset)) {
											fprintf(error_file,"ERROR - Wrong size of digital report %d, octet_offset %d\n",dataSetValue->value.octetString.size, octet_offset );
											MmsValue_delete(value);
											//LinkedList_destroy(attributes);
											return;
										}
										time_data time_value;
										time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
										time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
										time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
										time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];
										handle_event_report(dataSetValue->value.octetString.buf[octet_offset+RULE2_DIGITAL_REPORT_SIZE-1], index, time_value.t);
										octet_offset = octet_offset + RULE2_DIGITAL_REPORT_SIZE;
									} else if(dataset_conf[offset].type == DATASET_ANALOG){
										float_data data_value;
										if(dataSetValue->value.octetString.size < (RULE2_ANALOG_REPORT_SIZE + octet_offset)) {
											fprintf(error_file,"ERROR - Wrong size of analog report %d, octet_offset %d\n",dataSetValue->value.octetString.size, octet_offset );
											MmsValue_delete(value);
											//LinkedList_destroy(attributes);
											return;
										}
										//char float_string[5];
										data_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
										data_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
										data_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
										data_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];

										//MmsValue * analog_data = createAnalogValue(data_value.f, dataSetValue->value.octetString.buf[octet_offset+6]);

										handle_analog_report(data_value.f, dataSetValue->value.octetString.buf[octet_offset+6], index, time_stamp);

										octet_offset = octet_offset + RULE2_ANALOG_REPORT_SIZE;
									} else {
										fprintf(error_file,"ERROR - unkonwn information report index %d\n", index);
										int j;
										for (j=0; j < dataSetValue->value.octetString.size; j ++){
											printf(" %x", dataSetValue->value.octetString.buf[j]);
										}
										MmsValue_delete(value);
										//LinkedList_destroy(attributes);
										return;
									}
								}
							} else {
								fprintf(error_file,"ERROR - invalid RULE\n");
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
						}
					} else {
						fprintf(error_file,"ERROR - NULL Element\n");
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
	}
	//TODO: free values
}

static int read_configuration() {
	FILE * file = NULL;
	FILE * log_file = NULL;
	char line[300];
	int origin = 0;
	int event = 0;
	unsigned int nponto = 0;
	char id_ponto[25] = "";
	char type = 0;
	int i;

	//configuration = calloc(DATASET_MAX_SIZE*DATASET_MAX_NUMBER, sizeof(data_config) ); 
    analog = calloc(DATASET_MAX_SIZE*DATASET_ANALOG_MAX_NUMBER, sizeof(data_config) ); 
    digital = calloc(DATASET_MAX_SIZE*DATASET_DIGITAL_MAX_NUMBER, sizeof(data_config) ); 
    events  = calloc(DATASET_MAX_SIZE*DATASET_EVENTS_MAX_NUMBER, sizeof(data_config) ); 

	file = fopen(CONFIG_FILE, "r");
	if(file==NULL){
		printf("Error, cannot open configuration file %s\n", CONFIG_FILE);
		return -1;
	} else{
		//first two rows of CONFIG_FILE are the reader, discard them
		if(!fgets(line, 300, file) || !fgets(line, 300, file)){
			printf("Error reading %s file header\n", CONFIG_FILE);
			return -1;
		}

		while ( fgets(line, 300, file)){
			//if(sscanf(line, "%d %*d %22s %c", &configuration[num_of_ids].nponto,  configuration[num_of_ids].id, &configuration[num_of_ids].type ) <1)
			if(sscanf(line, "%d %*d %22s %c %*31s %*d %*d %*d %d %*c %*d %*d %*f %*f %*d %d", &nponto,  id_ponto, &type, &origin, &event ) <1)
				break;

			//change - for $
			for ( i=0; i <22; i++) {
				if (id_ponto[i] == '-' || id_ponto[i] == '+'){
					id_ponto[i] = '$';
				}
			}
	
			//Command Digital
			if(type == 'D' && origin ==7 ){
				//add $C to the end of command
				if (id_ponto[21] == 'K') {
					id_ponto[22] = '$';
					id_ponto[23] = 'C';
				}
				printf("command type for %s discarded\n", id_ponto);
			}//Command Analog
			else if(type == 'A' && origin ==7 ){
				//add $C to the end of command
				if (id_ponto[21] == 'K') {
					id_ponto[22] = '$';
					id_ponto[23] = 'C';
				}
				printf("command type for %s discarded\n", id_ponto);
			}
		   	//Events
			else if(type == 'D' && event == 3){
				memcpy(events[num_of_event_ids].id,id_ponto,25);
				events[num_of_event_ids].nponto = nponto;
				num_of_event_ids++;
			} //Digital
			else if(type == 'D'){
				memcpy(digital[num_of_digital_ids].id,id_ponto,25);
				digital[num_of_digital_ids].nponto = nponto;
				num_of_digital_ids++;
			} //Analog
			else if(type == 'A'){
				memcpy(analog[num_of_analog_ids].id,id_ponto,25);
				analog[num_of_analog_ids].nponto = nponto;
				num_of_analog_ids++;
			} //Unknown 
			else {
				printf("ERROR reading configuration file! Unknown type");
			}
		}
	}

	log_file = fopen(CONFIG_LOG, "w");
	if(log_file==NULL){
		printf("Error, cannot open configuration log file %s\n", CONFIG_LOG);
		fclose(file);
		return -1;
	} 

	fprintf(log_file, "***************ANALOG***********************\n");
	for (i=0; i < num_of_analog_ids; i++) {
		fprintf(log_file, "%d \t%s\t \n", analog[i].nponto,  analog[i].id);
	}

	fprintf(log_file, "***************DIGITAL***********************\n");
	for (i=0; i < num_of_digital_ids; i++) {
		fprintf(log_file, "%d \t%s\t \n", digital[i].nponto,  digital[i].id);
	}

	fprintf(log_file, "***************EVENTS***********************\n");
	for (i=0; i < num_of_event_ids; i++) {
		fprintf(log_file, "%d \t%s\t \n", events[i].nponto,  events[i].id);
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

static void sigint_handler(int signalId)
{
	running = 0;
}


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

}

int main (int argc, char ** argv){
	int i = 0;
	MmsError mmsError;
	signal(SIGINT, sigint_handler);
	MmsConnection con = MmsConnection_create();
		
	// READ CONFIGURATION FILE
	if (read_configuration() != 0) {
		printf("Error reading configuration\n");
		return -1;
	} else {
		printf("Start configuration with %d analog, %d digital, %d events and %d datasets\n", num_of_analog_ids, num_of_digital_ids, num_of_event_ids, num_of_datasets);
	}

	// OPEN DATA LOG	
#ifdef DATA_LOG
	data_file_analog = fopen(DATA_ANALOG_LOG, "w");
	if(data_file_analog==NULL){
		printf("Error, cannot open configuration data log file %s\n", DATA_ANALOG_LOG);
		cleanup_variables(con);
		return -1;
	}
	data_file_digital = fopen(DATA_DIGITAL_LOG, "w");
	if(data_file_digital==NULL){
		printf("Error, cannot open configuration data log file %s\n", DATA_DIGITAL_LOG);
		cleanup_variables(con);
		return -1;
	}
	data_file_events = fopen(DATA_EVENTS_LOG, "w");
	if(data_file_events==NULL){
		printf("Error, cannot open configuration data log file %s\n", DATA_EVENTS_LOG);
		cleanup_variables(con);
		return -1;
	}
#endif

	// OPEN ERROR LOG
	error_file = fopen(ERROR_LOG, "w");
	if(error_file==NULL){
		printf("Error, cannot open error log file %s\n", ERROR_LOG);
		fclose(error_file);
		return -1;
	}

	//INITIALIZE CONNECTION (TODO: resolve memory leak and add connection handler)
	if ((connect_to_server(con, SERVER_NAME_1) < 0) && (connect_to_server(con, SERVER_NAME_2) < 0) &&
	   	(connect_to_server(con, SERVER_NAME_3) < 0) && (connect_to_server(con, SERVER_NAME_4) < 0)) {
		cleanup_variables(con);
		return -1;
	}

	// DELETE DATASETS WHICH WILL BE USED
	printf("deleting dada sets\n");
	for (i=0; i < num_of_datasets; i++) {
		MmsConnection_deleteNamedVariableList(con,&mmsError, IDICCP, dataset_conf[i].id);
	}

	// CREATE TRASNFERSETS ON REMOTE SERVER	
	printf("creating transfer sets\n");
	for (i=0; i < num_of_datasets; i++){
		MmsValue *transfer_set_dig  = get_next_transferset(con, error_file);
		if( transfer_set_dig == NULL) {	
			printf("Could not create transfer set for digital data\n");
			cleanup_variables(con);
			return -1;
		} else {
			strncpy(dataset_conf[i].ts, MmsValue_toString(transfer_set_dig), TRANSFERSET_NAME_SIZE);
			//printf("New transfer set %s\n",dataset_conf[i].ts);
			MmsValue_delete(transfer_set_dig);
		}
	}	

	// CREATE DATASETS WITH CUSTOM VARIABLES
	printf("creating data sets\n");
	for (i=0; i < num_of_datasets; i++){
		create_dataset(con, dataset_conf[i].id,i);
	}

	// WRITE DATASETS TO TRANSFERSET
	printf("writing data sets\n");
	for (i=0; i < num_of_datasets; i++){
		if(dataset_conf[i].type == DATASET_ANALOG) 
			write_dataset(con, dataset_conf[i].id, dataset_conf[i].ts, DATASET_ANALOG_BUFFER_INTERVAL, DATASET_ANALOG_INTEGRITY_TIME, 0);
		else if(dataset_conf[i].type == DATASET_DIGITAL)
			write_dataset(con, dataset_conf[i].id, dataset_conf[i].ts, DATASET_DIGITAL_BUFFER_INTERVAL, DATASET_DIGITAL_INTEGRITY_TIME, 1);
		else if(dataset_conf[i].type == DATASET_EVENTS)
			write_dataset(con, dataset_conf[i].id, dataset_conf[i].ts, DATASET_EVENTS_BUFFER_INTERVAL, DATASET_EVENTS_INTEGRITY_TIME, 1);
		else{
			printf("unknown write dataset type\n");
			cleanup_variables(con);
			return -1;
		}

	}
	// READ VARIABLES
	printf("*************Reading DS Values********************\n");
	for (i=0; i < num_of_datasets; i++){
		read_dataset(con, dataset_conf[i].id, i);
	}

	// HANDLE REPORTS
	MmsConnection_setInformationReportHandler(con, informationReportHandler, (void *) con);

	// LOOP TO MANTAIN CONNECTION ACTIVE	
	while(running) {
		if (check_connection(con, error_file) < 0)
			break;
		sleep(2);//sleep 2s
	}

	cleanup_variables(con);

	return 0;
}
