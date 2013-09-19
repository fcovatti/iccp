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

#define CLIENT_DEBUG 1

static int running = 1;

//Configuration
static int num_of_ids = 0;	
static int num_of_datasets = 0;
static data_config * configuration = NULL;
static dataset_config * dataset_conf = NULL;

static int handle_analog_state(float value, char state, int index,time_t time_stamp ){
	configuration[index].f = value;
	configuration[index].state = state;
	configuration[index].time_stamp = time_stamp;
#ifdef CLIENT_DEBUG	
	printf("%23s: %04.3f |", configuration[index].id, value);
	print_value(state,1, time_stamp);
#endif
	return 0;
}

static int handle_digital_state(char state, int index, time_t time_stamp){
	configuration[index].state = state;
	configuration[index].time_stamp = time_stamp;
#ifdef CLIENT_DEBUG	
	printf("%23s: ", configuration[index].id);
	print_value(state,0, time_stamp);
#endif
	return 0;
}


static void sigint_handler(int signalId)
{
	running = 0;
}

static int read_data_set(MmsConnection con, char * ds_name, bool ana, int offset){
	MmsValue* dataSet;
	MmsClientError mmsError;
	int i;
	int number_of_variables = DATASET_MAX_SIZE; 

	//LASTA DATA SET HAS A SMALLER NUMBER
	if ( (offset+1) == num_of_datasets) {
		number_of_variables = num_of_ids - (DATASET_MAX_SIZE*offset);
	}
	dataSet = MmsConnection_readNamedVariableListValues(con, &mmsError, IDICCP, ds_name, 0);
	if (dataSet == NULL){
		printf("reading value failed! %d\n", mmsError);                                                                                                   
		return -1;
	}else{
		for (i=0; i < number_of_variables; i ++) {
			MmsValue* dataSetValue;
			MmsValue* dataSetElem;
			MmsValue* timeStamp;
			time_t time_stamp;

			//GET DATASET VALUES (FIRST 3 VALUES ARE ACCESS-DENIED)
			dataSetValue = MmsValue_getElement(dataSet, INDEX_OFFSET+i);
			if(dataSetValue == NULL) {
				printf("could not get DATASET values offset %d element %d %d \n",offset, i, number_of_variables);
				return -1;
			}

			if(configuration[i +(offset*DATASET_MAX_SIZE)].type == 'A'){
				MmsValue* analog_value;
				float analog_data;
				char analog_state;

				//First element Floating Point Value
				analog_value = MmsValue_getElement(dataSetValue, 0);
				if(analog_value == NULL) {
					printf("could not get floating point value\n");
					return -1;
				} 

				analog_data = MmsValue_toFloat(analog_value);

				//Second Element BitString data state
				analog_value = MmsValue_getElement(dataSetValue, 1);
				if(analog_value == NULL) {
					printf("could not get analog state\n");
					return -1;
				} 
				
				analog_state = analog_value->value.bitString.buf[0];
				time( &time_stamp);

				handle_analog_state(analog_data, analog_state, i +(offset*DATASET_MAX_SIZE), time_stamp);
			}else {
				//First element Data_TimeStampExtended
				dataSetElem = MmsValue_getElement(dataSetValue, 0);
				if(dataSetElem == NULL) {
					printf("could not get digital Data_TimeStampExtended\n");
					return -1;
				}
				timeStamp = MmsValue_getElement(dataSetElem, 0);

				if(timeStamp == NULL) {
					printf("could not get digital timestamp value\n");
					return -1;
				}
				time_stamp = MmsValue_toUint32(timeStamp);
				
				//Second Element DataState
				dataSetElem = MmsValue_getElement(dataSetValue, 1);
				if(dataSetElem == NULL) {
					printf("could not get digital DataState\n");
					return -1;
				}
				handle_digital_state(dataSetElem->value.bitString.buf[0], i +(offset*DATASET_MAX_SIZE), time_stamp);
			}
			MmsValue_delete(dataSetValue); 

		}

	}
	return 0;
}
static void write_data_set(MmsConnection con, char * ds_name, char * ts_name, int index) {
	//global
	MmsClientError mmsError;
	MmsTypeSpecification typeSpec;
	typeSpec.type = MMS_STRUCTURE;
	typeSpec.typeSpec.structure.elementCount = 13;
	typeSpec.typeSpec.structure.elements = calloc(13,
			sizeof(MmsTypeSpecification*));

	MmsTypeSpecification* element;

	//0
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->type = MMS_STRUCTURE;
	element->typeSpec.structure.elementCount = 3;
	element->typeSpec.structure.elements = calloc(3,
			sizeof(MmsTypeSpecification*));

	MmsTypeSpecification* inside_element;
	//0.0
	inside_element = calloc(1, sizeof(MmsTypeSpecification));
	inside_element->type = MMS_UNSIGNED;
	inside_element->typeSpec.unsignedInteger = 8;
	inside_element->typeSpec.structure.elementCount = 3;
	element->typeSpec.structure.elements[0] = inside_element;

	//0.1
	inside_element = calloc(1, sizeof(MmsTypeSpecification));
	inside_element->type = MMS_VISIBLE_STRING;
	inside_element->typeSpec.visibleString = -129;
	element->typeSpec.structure.elements[1] = inside_element;

	//0.2
	inside_element = calloc(1, sizeof(MmsTypeSpecification));
	inside_element->type = MMS_VISIBLE_STRING;
	inside_element->typeSpec.visibleString = -129;
	element->typeSpec.structure.elements[2] = inside_element;

	typeSpec.typeSpec.structure.elements[0] = element;

	//1
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec.typeSpec.structure.elements[1] = element;

	//2
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec.typeSpec.structure.elements[2] = element;

	//3
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec.typeSpec.structure.elements[3] = element;

	//4
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec.typeSpec.structure.elements[4] = element;

	//5
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec.typeSpec.structure.elements[5] = element;

	//6
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.bitString = 5;
	element->type = MMS_BIT_STRING;
	typeSpec.typeSpec.structure.elements[6] = element;

	//7
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec.typeSpec.structure.elements[7] = element;

	//8
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec.typeSpec.structure.elements[8] = element;

	//9
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec.typeSpec.structure.elements[9] = element;

	//10
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec.typeSpec.structure.elements[10] = element;

	//11
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->type = MMS_BOOLEAN;
	typeSpec.typeSpec.structure.elements[11] = element;

	//12
	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.integer = 8;
	element->type = MMS_INTEGER;
	typeSpec.typeSpec.structure.elements[12] = element;

	MmsValue* dataset = MmsValue_newStructure(&typeSpec);

	//0
	MmsValue* elem;
	elem = MmsValue_getElement(dataset, 0);

	//0.0
	MmsValue* ielem;
	ielem = MmsValue_getElement(elem, 0);
	MmsValue_setUint8(ielem, 1);

	//0.1
	ielem = MmsValue_getElement(elem, 1);
	MmsValue_setVisibleString(ielem, IDICCP);

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
	MmsValue_setInt32(elem, DATASET_BUFFER_INTERVAL);

	//5
	//Integrity check time
	elem = MmsValue_getElement(dataset, 5);
	MmsValue_setInt32(elem, DATASET_INTEGRITY_TIME);

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
	elem = MmsValue_getElement(dataset, 10);
	//MmsValue_setBoolean(elem, false);
	MmsValue_setBoolean(elem, true);

	//11
	elem = MmsValue_getElement(dataset, 11);
	MmsValue_setBoolean(elem, true);

	//12
	elem = MmsValue_getElement(dataset, 12);
	MmsValue_setInt32(elem, 0);

	MmsConnection_writeVariable(con, &mmsError, IDICCP, ts_name, dataset );
}

static void create_data_set(MmsConnection con, char * ds_name, bool ana, int offset)
{
	MmsClientError mmsError;
	int i=0;
	LinkedList variables = LinkedList_create();

	printf("create_data_Set %d\n", offset);
	MmsVariableSpecification * var; 
	MmsVariableSpecification * name = MmsVariableSpecification_create (IDICCP, "Transfer_Set_Name");
	MmsVariableSpecification * ts   = MmsVariableSpecification_create (IDICCP, "Transfer_Set_Time_Stamp");
	MmsVariableSpecification * ds   = MmsVariableSpecification_create (IDICCP, "DSConditions_Detected");
	LinkedList_add(variables, name );
	LinkedList_add(variables, ts);
	LinkedList_add(variables, ds);

	for (i = DATASET_MAX_SIZE*offset; (i<DATASET_MAX_SIZE*(offset+1)) &&( i < num_of_ids);i++){
		var = MmsVariableSpecification_create (IDICCP, configuration[i].id);
		var->arrayIndex = 0;	
		LinkedList_add(variables, var);
	}	

	MmsConnection_defineNamedVariableList(con, &mmsError, IDICCP, ds_name, variables); 

	LinkedList_destroy(variables);
}

static MmsValue * get_next_transfer_set(MmsConnection con)
{
//READ DATASETS
	MmsClientError mmsError;
	MmsValue* returnValue = NULL;
	MmsValue* value;
	value = MmsConnection_readVariable(con, &mmsError, IDICCP, "Next_DSTransfer_Set");
	
	if (value == NULL){                                                                                                                
		printf("reading transfer set value failed! %d\n", mmsError);                                                                                                   
		return NULL;
	}
	MmsValue* ts_elem;
	ts_elem = MmsValue_getElement(value, 0);
	if (ts_elem == NULL){
		printf("reading transfer set value 0 failed! %d\n", mmsError);                                                                                                   
		return NULL;
	}

	ts_elem = MmsValue_getElement(value, 1);
	if (ts_elem == NULL){
		printf("reading transfer set value 1 failed! %d\n", mmsError);                                                                                                   
		return NULL;
	} else {
		//printf("Read transfer set domain_id: %s\n", MmsValue_toString(ts_elem));
		if(strcmp(MmsValue_toString(ts_elem), IDICCP) != 0){
			printf("Wrong domain id\n");
			return NULL;
		}
	}

	ts_elem = MmsValue_getElement(value, 2);
	if (ts_elem == NULL){
		printf("reading transfer set value 2 failed! %d\n", mmsError);                                                                                                   
		return NULL;
	} else {
		printf("Read transfer set name: %s\n", MmsValue_toString(ts_elem));
		returnValue = MmsValue_newMmsString(MmsValue_toString(ts_elem));
	}
	MmsValue_delete(value); 
	return returnValue;
}
/*
static MmsValue * createAnalogValue(float data, char bitstring) {
	MmsValue* analog_data = NULL;
	MmsTypeSpecification typeSpec;
	typeSpec.type = MMS_STRUCTURE;
	typeSpec.typeSpec.structure.elementCount = 2;
	typeSpec.typeSpec.structure.elements = calloc(2,
			sizeof(MmsTypeSpecification*));
	MmsTypeSpecification* element;

	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.floatingpoint.formatWidth = 32;
	element->typeSpec.floatingpoint.exponentWidth = 8;
	element->type = MMS_FLOAT;
	typeSpec.typeSpec.structure.elements[0] = element;

	element = calloc(1, sizeof(MmsTypeSpecification));
	element->typeSpec.bitString = 5;
	element->type = MMS_BIT_STRING;
	typeSpec.typeSpec.structure.elements[1] = element;

	analog_data = MmsValue_newStructure(&typeSpec);

	MmsValue* elem;

	elem = MmsValue_getElement(analog_data, 0);
	MmsValue_setFloat(elem,data_value.f);

	elem = MmsValue_getElement(analog_data, 1);
	memcpy(elem->value.bitString.buf,bitstring, 1);
	elem->value.bitString.size = 5;
	return analog_data;
}
*/
static void
informationReportHandler (void* parameter, char* domainName, char* variableListName, MmsValue* value, LinkedList attributes, int attributesCount)
{
	int offset = 0;
	int i = 0;
	time_t time_stamp;
	time(&time_stamp);
	char * domain_id = NULL;
	char * transfer_set = NULL;
	MmsClientError mmsError;
	int octet_offset = 0;

	printf("*************Information Report Received********************\n");
	if (value != NULL && attributes != NULL && attributesCount) {
		//printf("received report no %i, attributesCount %d: ", rptCount++, attributesCount);
		LinkedList list_names	 = LinkedList_getNext(attributes);
		while (list_names != NULL) {
			char * attribute_name = (char *) list_names->data;
			list_names = LinkedList_getNext(list_names);
			MmsValue * dataSetValue = MmsValue_getElement(value, i);
			
			/*DEBUG
			if(dataSetValue != NULL){
				printf ("data set type %d , %d\n",dataSetValue->type, i);
			}*/

			if (strcmp("Transfer_Set_Name", attribute_name) == 0) {
				MmsValue* ts_name;
				MmsValue* d_id;
				if(dataSetValue !=NULL) {
					d_id = MmsValue_getElement(dataSetValue, 1);
					if(d_id !=NULL) {
						domain_id = MmsValue_toString(d_id);
					} else {
						printf("Empty domain id on report\n");
					}
					ts_name = MmsValue_getElement(dataSetValue, 2);
					if(ts_name !=NULL) {
						transfer_set = MmsValue_toString(ts_name);
					} else {
						printf("Empty transfer set name on report\n");
					}
				} else {
					printf("Empty data for transfer set report\n");
				}					
				i++;
				continue;
			}
			if (strcmp("Transfer_Set_Time_Stamp", attribute_name) == 0) {
				time_stamp =  MmsValue_toUint32(dataSetValue);
				i++;
				continue;
			}
			for (offset=0; offset<num_of_datasets;offset++) {
				if (strcmp(attribute_name,dataset_conf[offset].id) == 0) {
					if(dataSetValue != NULL){
						if (dataSetValue->value.octetString.buf != NULL) {
							short index = 0;	
							
							//DEBUG
							   /*int j;
							   for (j=0; j < dataSetValue->value.octetString.size; j ++){
							   printf(" %x", dataSetValue->value.octetString.buf[j]);
							   }
							   printf("\n"); */
						
							if(dataSetValue->value.octetString.buf[0] == 0) {
								if(dataSetValue->value.octetString.size == 0) {
									printf("Error - empty octetString\n");
									return;
								}

								octet_offset = 1; //first byte is the rule
								index = offset*DATASET_MAX_SIZE; // config index

								while (octet_offset < dataSetValue->value.octetString.size){
									if (configuration[index].type == 'D') {
										time_data time_value;
										time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
										time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
										time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
										time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
										handle_digital_state(dataSetValue->value.octetString.buf[octet_offset+6], index, time_value.t);
										octet_offset = octet_offset + 7;
									}else if (configuration[index].type == 'A') {
										float_data data_value;
										data_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
										data_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
										data_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
										data_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
										handle_analog_state(data_value.f, dataSetValue->value.octetString.buf[octet_offset+4], index, time_stamp);
										octet_offset = octet_offset + 5;
									} else {
										printf("wrong index %d\n", index);
										return;
									}
									index++;
								}

								//RULE 0
								//TODO: implement rule 0 handling
							} 
							if(dataSetValue->value.octetString.buf[0] == 1) {
								//RULE 1 - not implemented
								printf("Error - Information Report with rule 1 received\n");
								return;
							}
							else if (dataSetValue->value.octetString.buf[0] == 2) {
								//RULE 2
								if(dataSetValue->value.octetString.size == 0) {
									printf("Error - empty octetString\n");
									return;
								}

								octet_offset = 1; //first byte is the rule

								while (octet_offset < dataSetValue->value.octetString.size){
									// Packet INDEX
									index = dataSetValue->value.octetString.buf[octet_offset]<<8 | (dataSetValue->value.octetString.buf[octet_offset+1]-INDEX_OFFSET);
									// Translate into configuration index
									index = index + (offset*DATASET_MAX_SIZE);
									if (configuration[index].type == 'D') {
										if(dataSetValue->value.octetString.size < (RULE2_DIGITAL_REPORT_SIZE+octet_offset)) {
											printf("Wrong size of digital report %d, octet_offset %d\n",dataSetValue->value.octetString.size, octet_offset );
											return;
										}
										time_data time_value;
										time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
										time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
										time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
										time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];
										handle_digital_state(dataSetValue->value.octetString.buf[octet_offset+RULE2_DIGITAL_REPORT_SIZE-1], index, time_value.t);
										octet_offset = octet_offset + RULE2_DIGITAL_REPORT_SIZE;
									} else if (configuration[index].type == 'A'){
										float_data data_value;
										if(dataSetValue->value.octetString.size < (RULE2_ANALOG_REPORT_SIZE + octet_offset)) {
											printf("Wrong size of analog report %d, octet_offset %d\n",dataSetValue->value.octetString.size, octet_offset );
											return;
										}
										//char float_string[5];
										data_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
										data_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
										data_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
										data_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];

										//MmsValue * analog_data = createAnalogValue(data_value.f, dataSetValue->value.octetString.buf[octet_offset+6]);

										handle_analog_state(data_value.f, dataSetValue->value.octetString.buf[octet_offset+6], index, time_stamp);

										octet_offset = octet_offset + RULE2_ANALOG_REPORT_SIZE;
									} else {
										printf("unkonwn information report index %d\n", index);
										int j;
										for (j=0; j < dataSetValue->value.octetString.size; j ++){
											printf(" %x", dataSetValue->value.octetString.buf[j]);
										}
										printf("\n"); 
										return;
									}
								}
							}
							} else {
								printf("empty bitstring\n");
							}
						} else {
							printf(" NULL Element\n");
						}
					}
				}
				i++;
			}
		printf("\n");

		//CONFIRM RECEPTION OF EVENT
		MmsConnection_sendUnconfirmedPDU((MmsConnection)parameter,&mmsError,domain_id, transfer_set, time_stamp);
	} else{
		printf("wrong report\n");
	}
	//TODO: free values
}

#if 0
//GET domain list
////		nameList = MmsConnection_getDomainNames(con, &mmsError);
   //     nameList = MmsConnection_getDomainVariableNames(con, &mmsError, "TESTE/MAR04");
		//nameList = MmsConnection_readNamedVariableListDirectory (con, "TESTE", "SAGE_ANL_4", &deletable); 
//		nameList = MmsConnection_getVariableListNamesAssociationSpecific(con, &mmsError);
//      nameList = MmsConnection_getDomainVariableListNames(con, "TESTE");
		
//		MmsConnection_getVariableAccessAttributes(con, "TESTE", "CBO$AL1$$MAPH$$B");
//		value = MmsConnection_readVariable(con, &mmsError, "TESTE/MAR04", "CBO$AL1$$MAPH$$B");

/*
*/

// gets varibles
#if 0
		MmsValue* value;
		value = MmsConnection_readVariable(con, &mmsError, "COS_A", "ds_001_dig");
		if (value == NULL)                                                                                                                       
			printf("reading value failed! %d\n", mmsError);                                                                                                   
		else{                                                                                                                                     
			printf("Read variable with value: %f\n", MmsValue_toFloat(value));
	 		MmsValue_delete(value); 
		}
		LinkedList nameList = mmsClient_getNameList(con, &mmsError, NULL, 0, 0);
		LinkedList_printStringList(nameList);
#endif
			//		printf("Got domain names\n");
		/*		if(nameList == NULL) {
				printf("empty nameList %d\n", mmsError);
				return 0;
				}
		 */
		//	LinkedList_printStringList(nameList);

		/*	
			while((nameList = LinkedList_getNext(nameList)) != NULL){
			char *str = (char *) (nameList->data);
			printf("%s\n", str);
			}*/

#endif
static int read_configuration() {
	FILE * file;
	FILE * log_file;
	char line[300];

	configuration = malloc(sizeof(data_config)*DATASET_MAX_SIZE*DATASET_MAX_NUMBER); 

	file = fopen(CONFIG_FILE, "r");
	if(file==NULL){
		printf("Error, cannot open configuration file %s\n", CONFIG_FILE);
		return -1;
	} else{
		while ( fgets(line, 300, file)){
			if(sscanf(line, "%d %*d %22s %c", &configuration[num_of_ids].nponto,  configuration[num_of_ids].id, &configuration[num_of_ids].type ) <1)
				break;
			num_of_ids++;
		}
	}

	log_file = fopen(CONFIG_LOG, "w");
	if(log_file==NULL){
		printf("Error, cannot open configuration log file %s\n", CONFIG_LOG);
		fclose(file);
		return -1;
	} 
	int i,j;
	for (j=0; j < num_of_ids; j++) {
		for ( i=0; i <22; i++) {
			if (configuration[j].id[i] == '-'){
				configuration[j].id[i] = '$';
			}
		}
		fprintf(log_file, "%d \t%s\t %c\n", configuration[j].nponto,  configuration[j].id, configuration[j].type);
	}

	num_of_datasets = num_of_ids/DATASET_MAX_SIZE;
	if(num_of_ids%DATASET_MAX_SIZE)
		num_of_datasets++;

	//alloc data for datasets
	dataset_conf = malloc(sizeof(dataset_config)*num_of_datasets);
	
	for (i=0; i < num_of_datasets; i++) {
		snprintf(dataset_conf[i].id, DATASET_NAME_SIZE, "ds_%03d", i);
	}
	fclose(file);
	fclose(log_file);
	return 0;
}

int main (int argc, char ** argv){
	int i = 0;
	int loop_error = 0;
	MmsClientError mmsError;
	MmsConnection con = MmsConnection_create();
		signal(SIGINT, sigint_handler);

	// READ CONFIGURATION FILE
	if (read_configuration(configuration, &num_of_ids) != 0) {
		printf("Error reading configuration\n");
		return -1;
	} else {
		printf("Start configuration with %d points and %d datasets\n", num_of_ids, num_of_datasets);
	}

	//INITIALIZE CONNECTION
	MmsIndication indication = MmsConnection_connect(con, &mmsError, SERVER_NAME, 102);
	if (indication == MMS_OK){
		printf("Connection OK!!!\n");

		// DELETE DATASETS WHICH WILL BE USED
		for (i=0; i < num_of_datasets; i++) {
			MmsConnection_deleteNamedVariableList(con,&mmsError, IDICCP, dataset_conf[i].id);
		}

		// CREATE TRASNFERSETS ON REMOTE SERVER	
		for (i=0; i < num_of_datasets; i++){
			MmsValue *transfer_set_dig  = get_next_transfer_set(con);
			if( transfer_set_dig == NULL) {	
				printf("Could not create transfer set for digital data\n");
				return -1;
			} else {
				strncpy(dataset_conf[i].ts, MmsValue_toString(transfer_set_dig), TRANSFERSET_NAME_SIZE);
				printf("New transfer set %s\n",dataset_conf[i].ts);
			}
		}	
		
		// CREATE DATASETS WITH CUSTOM VARIABLES
		for (i=0; i < num_of_datasets; i++){
			create_data_set(con, dataset_conf[i].id, 0, i);
		}
		// WRITE DATASETS TO TRANSFERSET
		for (i=0; i < num_of_datasets; i++){
			write_data_set(con, dataset_conf[i].id, dataset_conf[i].ts, i);
		}
		// READ VARIABLES
		printf("*************Reading DS Values********************\n");
		for (i=0; i < num_of_datasets; i++){
			read_data_set(con, dataset_conf[i].id, 0, i);
		}
		// HANDLE REPORTS
		MmsConnection_setInformationReportHandler(con, informationReportHandler, (void *) con);
		
		// LOOP TO MANTAIN CONNECTION ACTIVE	
		MmsValue* loop_value;
		while(running) {
			loop_value = MmsConnection_readVariable(con, &mmsError, IDICCP, "Bilateral_Table_ID");
			if (loop_value == NULL){ 
				if (mmsError == MMS_ERROR_CONNECTION_LOST){
					printf("Connection lost\n");
					return -1;
				}
				loop_error++;
				if(loop_error <3){
					printf("loop error %d reading value - mmsError %d\n", loop_error, mmsError);
				}else {
					printf("aborting due to consecutive loop errors reading value - mmsError %d\n", mmsError);
					return -1;
				}
			}else{                                                                                                                                     
				loop_error=0;
			}
			sleep(1);
		}

	}else {
		printf("Connection Error %d %d", indication, mmsError);
	}

	MmsConnection_destroy(con);
	return 0;
}
