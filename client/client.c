#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include "mms_client_connection.h"
//report handler

typedef struct {
	unsigned int nponto;
	char id[22];
	char type;
} data_config;

typedef union {
	float f;
	char  s[4];
} float_data;

//static int rptCount = 0;

static int running = 1;

static char * IDICCP = "COS_A";
static char * DS_DIG = "ds_001_dig";
static char * DS_ANA = "ds_001_anl";
static char * config_file = "sage_id_no_155.txt";
static char * config_log = "iccp_config.log";
#define INDEX_OFFSET 3
#define ANALOG_REPORT_SIZE 8
#define DIGITAL_REPORT_SIZE 10

//Configuration
int num_of_ids = 0;	
data_config * configuration = NULL;

static void print_value (MmsValue * value, bool ana, time_t time_stamp) {

			struct tm * time_result;
/*	printf("State_hi %d State_lo %d, Validity_hi %d, Validity_lo %d, CurrentSource_hi %d, CurrentSource_lo %d, NormalValue %d, TimeStampQuality %d \n", 
				MmsValue_getBitStringBit(value,0),
				MmsValue_getBitStringBit(value,1),
				MmsValue_getBitStringBit(value,2),
				MmsValue_getBitStringBit(value,3),
				MmsValue_getBitStringBit(value,4),
				MmsValue_getBitStringBit(value,5),
				MmsValue_getBitStringBit(value,6),
				MmsValue_getBitStringBit(value,7) );
*/
	if (!ana) {
		//ESTADO
		if (MmsValue_getBitStringBit(value,0) && !MmsValue_getBitStringBit(value,1)) {
			printf("Estado Ligado |");
		}else if (!MmsValue_getBitStringBit(value,0) && MmsValue_getBitStringBit(value,1)) {
			printf("Estado Desligado |");
		} else {
			printf ("Estado Invalido |");
		}
	}

		//Validade
		if (!MmsValue_getBitStringBit(value,2) && !MmsValue_getBitStringBit(value,3)) {
			printf("Valido |");
		}else if (!MmsValue_getBitStringBit( value,2) && MmsValue_getBitStringBit(value,3)) {
			printf("Segurado (Held) |");
		}else if (MmsValue_getBitStringBit(value,2) && !MmsValue_getBitStringBit(value,3)) {
			printf("Suspeito (Suspect) |");
		} else {
			printf ("Inválido |");
		}

		// Origem
		if (!MmsValue_getBitStringBit(value,4) && !MmsValue_getBitStringBit(value,5)) {
			printf("Telemedido |");
		}else if (!MmsValue_getBitStringBit(value,4) && MmsValue_getBitStringBit(value,5)) {
			printf("Calculado |");
		}else if (MmsValue_getBitStringBit(value,4) && MmsValue_getBitStringBit(value,5)) {
			printf("Manual |");
		} else {
			printf ("Estimado |");
		}

		// Valor Normal
		if (!MmsValue_getBitStringBit(value,6)){
			printf ("Normal |");
		} else {
			printf ("Anormal |");
		}

		// Estampa de tempo
		if (!MmsValue_getBitStringBit(value,7)){
			printf ("Estampa de t Valida |");
		} else {
			printf ("Estampa de t Invalida |");
		}

		time_result = localtime(&time_stamp);
		printf("TimeStamp %s ", asctime(time_result));

}

static int handle_analog_state(MmsValue * analog_value,int index,time_t time_stamp ){
	MmsValue* value;

	if(analog_value == NULL){
		printf("Error null analog value\n");
		return -1;
	}
	//First element Floating Point Value
	value = MmsValue_getElement(analog_value, 0);
	if(value == NULL) {
		printf("could not get floating point value\n");
		return -1;
	} else {
		printf("%s: %.2f |", configuration[index].id, MmsValue_toFloat(value));

	}

	//Second Element BitString data state
	value = MmsValue_getElement(analog_value, 1);
	if(value == NULL) {
		printf("could not get analog state\n");
		return -1;
	}
	
	print_value(value,1, time_stamp);

		return 0;
}

static int handle_digital_state(MmsValue * value, int index, time_t time_stamp){
	if(value == NULL){
		printf("Error null digital state value\n");
		return -1;
	}
	printf("%s: ", configuration[index].id);
	print_value(value,0, time_stamp);
		return 0;
}


static void sigint_handler(int signalId)
{
	running = 0;
}

static int read_data_set(MmsConnection con, char * ds_name, bool ana){
	MmsValue* dataSet;
	MmsClientError mmsError;
	int i;
	int number_of_variables = 25; //TODO get from config file
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
			dataSetValue = MmsValue_getElement(dataSet, 3+i);
			if(dataSetValue == NULL) {
				printf("could not get DATASET values\n");
				return -1;
			}

			if(configuration[i].type == 'A'){
				time( &time_stamp);
				handle_analog_state(dataSetValue, i, time_stamp);
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
				handle_digital_state(dataSetElem, i, time_stamp);
			}
			MmsValue_delete(dataSetValue); 

		}

	}
	return 0;
}
static void write_data_set(MmsConnection con, char * ds_name, char * ts_name) {
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
	elem = MmsValue_getElement(dataset, 4);
	MmsValue_setInt32(elem, 1);

	//5
	elem = MmsValue_getElement(dataset, 5);
	MmsValue_setInt32(elem, 1800);

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
	elem = MmsValue_getElement(dataset, 10);
	MmsValue_setBoolean(elem, true);

	//11
	elem = MmsValue_getElement(dataset, 11);
	MmsValue_setBoolean(elem, true);

	//12
	elem = MmsValue_getElement(dataset, 12);
	MmsValue_setInt32(elem, 0);

	MmsConnection_writeVariable(con, &mmsError, IDICCP, "COS_A_ts_001", dataset );
}

static void create_data_set(MmsConnection con, char * ds_name, bool ana)
{
	MmsClientError mmsError;
	int i=0;
	LinkedList variables = LinkedList_create();

	MmsVariableSpecification * var; 
	MmsVariableSpecification * name = MmsVariableSpecification_create (IDICCP, "Transfer_Set_Name");
	MmsVariableSpecification * ts   = MmsVariableSpecification_create (IDICCP, "Transfer_Set_Time_Stamp");
	MmsVariableSpecification * ds   = MmsVariableSpecification_create (IDICCP, "DSConditions_Detected");
	LinkedList_add(variables, name );
	LinkedList_add(variables, ts);
	LinkedList_add(variables, ds);

	for (i = 0; i<25;i++){
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
static void
informationReportHandler (void* parameter, char* domainName, char* variableListName, MmsValue* value, LinkedList attributes, int attributesCount)
{
	int i = 0;
	time_t time_stamp;
	time(&time_stamp);
	printf("*************Information Report Received********************\n");
	if (value != NULL && attributes != NULL && attributesCount) {
		//printf("received report no %i, attributesCount %d: ", rptCount++, attributesCount);
		LinkedList list_names	 = LinkedList_getNext(attributes);
		while (list_names != NULL) {
			char * attribute_name = (char *) list_names->data;
			//printf("Attributes %s\n", attribute_name);
			list_names = LinkedList_getNext(list_names);
			MmsValue * dataSetValue = MmsValue_getElement(value, i);
			
			/*if(dataSetValue != NULL){
				printf ("data set type %d , %d\n",dataSetValue->type, i);
			}*/
			if (strcmp("Transfer_Set_Time_Stamp", attribute_name) == 0) {

				time_stamp =  MmsValue_toUint32(dataSetValue);

			}
			if (strcmp(DS_DIG, attribute_name) == 0) {
				if(dataSetValue != NULL){
					if (dataSetValue->value.octetString.buf != NULL) {
						
						//DEBUG
						/*
						int j;
						for (j=0; j < dataSetValue->value.octetString.size; j ++){
							printf(" %x", dataSetValue->value.octetString.buf[j]);
						}
						printf("\n"); */
						// create MmsValue
						if (configuration[dataSetValue->value.octetString.buf[2]-INDEX_OFFSET].type == 'D') {
							if(dataSetValue->value.octetString.size != DIGITAL_REPORT_SIZE) {
								printf("Wrong size of digital report %d\n",dataSetValue->value.octetString.size );
								return;
							}
							MmsValue * digital_state = MmsValue_newBitString(8);
							memcpy(digital_state->value.bitString.buf, &dataSetValue->value.octetString.buf[dataSetValue->value.octetString.size-1], 1);
							handle_digital_state(digital_state, dataSetValue->value.octetString.buf[2]-INDEX_OFFSET, time_stamp);
						} else if (configuration[dataSetValue->value.octetString.buf[2]-INDEX_OFFSET].type == 'A'){
							float_data data_value;
							if(dataSetValue->value.octetString.size != ANALOG_REPORT_SIZE) {
								printf("Wrong size of analog report %d\n",dataSetValue->value.octetString.size );
								return;
							}
							//char float_string[5];
							data_value.s[3] = dataSetValue->value.octetString.buf[3];
							data_value.s[2] = dataSetValue->value.octetString.buf[4];
							data_value.s[1] = dataSetValue->value.octetString.buf[5];
							data_value.s[0] = dataSetValue->value.octetString.buf[6];

							//printf("Report id %s, float %f size %ld\n",configuration[dataSetValue->value.octetString.buf[2]-INDEX_OFFSET].id, data_value.f , sizeof(float));
							//MmsValue * analog_value = MmsValue_newFloat(data_value.f);
							//MmsValue * analog_state = MmsValue_newBitString(dataSetValue->value.octetString.buf[7]);

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


							MmsValue* analog_data = MmsValue_newStructure(&typeSpec);

							MmsValue* elem;
													
							elem = MmsValue_getElement(analog_data, 0);
							MmsValue_setFloat(elem,data_value.f);

							elem = MmsValue_getElement(analog_data, 1);
							memcpy(elem->value.bitString.buf, &dataSetValue->value.octetString.buf[7], 1);
							elem->value.bitString.size = 5;
							
							handle_analog_state(analog_data, dataSetValue->value.octetString.buf[2]-INDEX_OFFSET, time_stamp);
						} else {
							printf("unkonwn information report\n");
						}
					} else {
						printf("empty bitstring\n");
					}
				} else {
					printf(" NULL Element\n");
				}
			}else if (strcmp(DS_ANA, attribute_name) == 0) {
				printf("Received analog report\n");
				if(dataSetValue != NULL){
					printf ("data set type %d , %d\n",dataSetValue->type, i);
				}
			}

			i++;

		}
		printf("\n");
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

	printf("alloced configuration\n");
	configuration = malloc(sizeof(data_config)*100000); //100.000 pontos

	file = fopen(config_file, "r");
	if(file==NULL){
		printf("Error, cannot open configuration file %s\n", config_file);
		return -1;
	} else{
		while ( fgets(line, 300, file)){
			if(fscanf(file, "%d %*d %s %c", &configuration[num_of_ids].nponto,  configuration[num_of_ids].id, &configuration[num_of_ids].type ) <1)
				break;
			num_of_ids++;
		}
	}

	printf("scaned configuration\n");
	//replace caracthers
	//

	log_file = fopen(config_log, "w");
	if(log_file==NULL){
		printf("Error, cannot open configuration log file %s\n", config_log);
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

	printf("finisherd configuration\n");
	fclose(file);
	fclose(log_file);
	return 0;
}
int main (int argc, char ** argv){
	MmsClientError mmsError;
	MmsConnection con = MmsConnection_create();
		signal(SIGINT, sigint_handler);

	//READ CONFIGURATION FILE
	if (read_configuration(configuration, &num_of_ids) != 0) {
		printf("Error reading configuration\n");
		return -1;
	} else {
		printf("Start configuration with %d points\n", num_of_ids);
	}

	//INITIALIZE Connection
	MmsIndication indication = MmsConnection_connect(con, &mmsError, "cems1", 102);
	if (indication == MMS_OK){
		printf("Conexão OK!!!\n");

		// DELETE DATASETS WHICH WILL BE USED
		MmsConnection_deleteNamedVariableList(con,&mmsError, IDICCP, DS_DIG);
		//MmsConnection_deleteNamedVariableList(con,&mmsError, IDICCP, DS_ANA);

		// CREATE TRASNFERSETS ON REMOTE SERVER	
		MmsValue *transfer_set_dig  = get_next_transfer_set(con);
		if( transfer_set_dig == NULL) {	
			printf("Could not create transfer set for digital data\n");
			return -1;
		} else {
			printf("New transfer set %s\n",MmsValue_toString(transfer_set_dig));
		}
		/*MmsValue *transfer_set_ana  = get_next_transfer_set(con);
		if( transfer_set_ana == NULL) {	
			printf("Could not create transfer set for analog data\n");
			return -1;
		}else {
			printf("New transfer set %s\n", MmsValue_toString(transfer_set_ana));
		}*/

		// CREATE DATASETS WITH CUSTOM VARIABLES
		create_data_set(con, DS_DIG, 0);
		//create_data_set(con, DS_ANA, 1);

		//WRITE DATASETS TO TRANSFERSET
		write_data_set(con, DS_DIG, MmsValue_toString(transfer_set_dig));
		//write_data_set(con, DS_ANA, MmsValue_toString(transfer_set_ana));

		// READ VARIABLES
		printf("*************Reading Digital Values********************\n");
		read_data_set(con, DS_DIG, 0);

		printf("*************Reading Analog Values********************\n");
		//read_data_set(con, DS_ANA, 1);

		// HANDLE REPORTS
		MmsConnection_setInformationReportHandler(con, informationReportHandler, NULL);
		
		//LOOP TO MANTAIN CONNECTION	
		MmsValue* loop_value;
		while(running) {
			loop_value = MmsConnection_readVariable(con, &mmsError, IDICCP, "Bilateral_Table_ID");
			if (loop_value == NULL){ 
				printf("reading value failed! %d\n", mmsError);                                                                                                   
				continue;
			}else{                                                                                                                                     
				sleep(1);
			}
		}

	}else {
		printf("erro de conexão %d %d", indication, mmsError);
	}

	MmsConnection_destroy(con);
	return 0;
}
