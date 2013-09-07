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
static int rptCount = 0;

static int running = 1;

static void sigint_handler(int signalId)
{
	running = 0;
}
static int handle_digital_state(MmsValue * value){
	if(value == NULL){
		printf("Error null digital state value\n");
		return -1;
	}
	printf("State_hi %d State_lo %d, Validity_hi %d, Validity_lo %d, CurrentSource_hi %d, CurrentSource_lo %d, NormalValue %d, TimeStampQuality %d \n", 
				MmsValue_getBitStringBit(value,0),
				MmsValue_getBitStringBit(value,1),
				MmsValue_getBitStringBit(value,2),
				MmsValue_getBitStringBit(value,3),
				MmsValue_getBitStringBit(value,4),
				MmsValue_getBitStringBit(value,5),
				MmsValue_getBitStringBit(value,6),
				MmsValue_getBitStringBit(value,7) );

		//ESTADO
		if (MmsValue_getBitStringBit(value,0) && !MmsValue_getBitStringBit(value,1)) {
			printf("Estado Ligado\n");
		}else if (!MmsValue_getBitStringBit(value,0) && MmsValue_getBitStringBit(value,1)) {
			printf("Estado Desligado\n");
		} else {
			printf ("Estado Invalido\n");
		}

		//Validade
		if (!MmsValue_getBitStringBit(value,2) && !MmsValue_getBitStringBit(value,3)) {
			printf("Ponto Valido\n");
		}else if (!MmsValue_getBitStringBit( value,2) && MmsValue_getBitStringBit(value,3)) {
			printf("Ponto Segurado (Held)\n");
		}else if (MmsValue_getBitStringBit(value,2) && !MmsValue_getBitStringBit(value,3)) {
			printf("Ponto Suspeito (Suspect)\n");
		} else {
			printf ("Ponto Inválido\n");
		}

		// Origem
		if (!MmsValue_getBitStringBit(value,4) && !MmsValue_getBitStringBit(value,5)) {
			printf("Ponto Telemedido\n");
		}else if (!MmsValue_getBitStringBit(value,4) && MmsValue_getBitStringBit(value,5)) {
			printf("Ponto Calculado\n");
		}else if (MmsValue_getBitStringBit(value,4) && MmsValue_getBitStringBit(value,5)) {
			printf("Ponto Manual\n");
		} else {
			printf ("Ponto Estimado\n");
		}

		// Valor Normal
		if (!MmsValue_getBitStringBit(value,6)){
			printf ("Ponto Normal\n");
		} else {
			printf ("Ponto Anormal\n");
		}

		// Estampa de tempo
		if (!MmsValue_getBitStringBit(value,7)){
			printf ("Estampa de Tempo Valida\n");
		} else {
			printf ("Estampa de Tempo Invalida\n");
		}

		return 0;
}
static void
informationReportHandler (void* parameter, char* domainName, char* variableListName, MmsValue* value, LinkedList attributes, int attributesCount)
{
	int i = 0,j;
	if (value != NULL && attributes != NULL && attributesCount) {
		printf("received report no %i, attributesCount %d: ", rptCount++, attributesCount);
		LinkedList list_names	 = LinkedList_getNext(attributes);
		while (list_names != NULL) {
			char * attribute_name = (char *) list_names->data;
			printf("Attributes %s\n", attribute_name);
			list_names = LinkedList_getNext(list_names);
			MmsValue * dataSetValue = MmsValue_getElement(value, i);
			
			/*if(dataSetValue != NULL){
				printf ("data set type %d , %d\n",dataSetValue->type, i);
			}*/
			if (strcmp("Transfer_Set_Time_Stamp", attribute_name) == 0) {

				printf("Read variable with value: %d\n", MmsValue_toUint32(dataSetValue));
			}
			if (strcmp("ds_001_dig", attribute_name) == 0) {
				if(dataSetValue != NULL){
					if (dataSetValue->value.octetString.buf != NULL) {
						for (j=0; j < dataSetValue->value.octetString.size; j ++){
							printf(" %x", dataSetValue->value.octetString.buf[j]);
						}
						printf("\n");
						// create MmsValue
						MmsValue * digital_state = MmsValue_newBitString(8);
						memcpy(digital_state->value.bitString.buf, &dataSetValue->value.octetString.buf[dataSetValue->value.octetString.size-1], 1);
						handle_digital_state(digital_state);
					} else {
						printf("empty bitstring\n");
					}
				} else {
					printf(" NULL Element\n");
				}
			}
			i++;

		}
/*		MmsValue* inclusionField = MmsValue_getElement(value, 3);

		if (MmsValue_getBitStringBit(inclusionField, 4)) {
			int valueIndex = MmsValue_getArraySize(value) - 1;

			MmsValue* totW$Mag = MmsValue_getElement(value, valueIndex);

			MmsValue* totW$Mag$f = MmsValue_getElement(totW$Mag, 0);

			float measuredValue = MmsValue_toFloat(totW$Mag$f);

			printf("MMXU1.TotW.mag.f = %f", measuredValue);
		}
*/
		printf("\n");
	} else{
		printf("wrong report\n");
	}
	//MmsValue_delete(value);
	//TODO: free values
}


int main (int argc, char ** argv){



//	LinkedList nameList;
	MmsClientError mmsError;
	int i = 0;
	int number_of_variables = 1;
	MmsConnection con = MmsConnection_create();

	signal(SIGINT, sigint_handler);
	//bool deletable;

//	MmsIndication indication = MmsConnection_connect(con, "cems1", 102);
	MmsIndication indication = MmsConnection_connect(con, &mmsError, "cems1", 102);
	if (indication == MMS_OK){
		printf("Conexão OK!!!\n");
//		nameList = MmsConnection_getDomainNames(con, &mmsError);
   //     nameList = MmsConnection_getDomainVariableNames(con, &mmsError, "TESTE/MAR04");
		//nameList = MmsConnection_readNamedVariableListDirectory (con, "TESTE", "SAGE_ANL_4", &deletable); 
//		nameList = MmsConnection_getVariableListNamesAssociationSpecific(con, &mmsError);
//      nameList = MmsConnection_getDomainVariableListNames(con, "TESTE");
		
//		MmsConnection_getVariableAccessAttributes(con, "TESTE", "CBO$AL1$$MAPH$$B");
//		value = MmsConnection_readVariable(con, &mmsError, "TESTE/MAR04", "CBO$AL1$$MAPH$$B");

/*
		MmsValue* value;
		value = MmsConnection_readVariable(con, &mmsError, "COS_A", "Bilateral_Table_ID");
		if (value == NULL)                                                                                                                       
			printf("reading value failed! %d\n", mmsError);                                                                                                   
		else{                                                                                                                                     
			printf("Read variable with value: %f\n", MmsValue_toFloat(value));
	 		MmsValue_delete(value); 
		}
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

		// DELETE DATASETS WHICH WILL BE USED
	MmsConnection_deleteNamedVariableList(con,&mmsError, "COS_A", "ds_001_dig");
	MmsConnection_deleteNamedVariableList(con,&mmsError, "COS_A", "ds_001_anl");
	
	//READ DATASETS
	MmsValue* value;
	value = MmsConnection_readVariable(con, &mmsError, "COS_A", "Next_DSTransfer_Set");
	
	
	if (value == NULL){                                                                                                                
		printf("reading transfer set value failed! %d\n", mmsError);                                                                                                   
		return -1;
	}else{                                                                                                                                     
		MmsValue* ts_elem;
		ts_elem = MmsValue_getElement(value, 0);
		if (ts_elem == NULL){
			printf("reading transfer set value 0 failed! %d\n", mmsError);                                                                                                   
			return -1;
		} else {
			printf("Read Digital transfer set index: %d\n", MmsValue_toUint32(ts_elem));
		}
		
		ts_elem = MmsValue_getElement(value, 1);
		if (ts_elem == NULL){
			printf("reading transfer set value 1 failed! %d\n", mmsError);                                                                                                   
			return -1;
		} else {
			printf("Read Digital transfer set domain_id: %s\n", MmsValue_toString(ts_elem));
		}

		ts_elem = MmsValue_getElement(value, 2);
		if (ts_elem == NULL){
			printf("reading transfer set value 2 failed! %d\n", mmsError);                                                                                                   
			return -1;
		} else {
			printf("Read Digital transfer set name: %s\n", MmsValue_toString(ts_elem));
		}
		MmsValue_delete(value); 
	}


	//CREATE DATASETS
	LinkedList variables = LinkedList_create();
	MmsVariableSpecification * name = MmsVariableSpecification_create ("COS_A", "Transfer_Set_Name");
	MmsVariableSpecification * ts   = MmsVariableSpecification_create ("COS_A", "Transfer_Set_Time_Stamp");
	MmsVariableSpecification * ds   = MmsVariableSpecification_create ("COS_A", "DSConditions_Detected");
	MmsVariableSpecification * cbo  = MmsVariableSpecification_create ("COS_A", "CBO$AL6$$XCBR5233");
	MmsVariableSpecification * dbo  = MmsVariableSpecification_create ("COS_A", "CBO$AL6$$XCMD5233");
	cbo->arrayIndex = 0;	
	dbo->arrayIndex = 0;	

	LinkedList_add(variables, name );
	LinkedList_add(variables, ts);
	LinkedList_add(variables, ds);
	LinkedList_add(variables, cbo);
	LinkedList_add(variables, dbo);

	MmsConnection_defineNamedVariableList(con, &mmsError, "COS_A", "ds_001_dig", variables); 

	LinkedList_destroy(variables);

	//WRITE DATASETS TO TRANSFERSETS
	//global
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
	MmsValue_setVisibleString(ielem, "COS_A");
	
	//0.2
	ielem = MmsValue_getElement(elem, 2);
	MmsValue_setVisibleString(ielem, "ds_001_dig");

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
	
	MmsConnection_writeVariable(con, &mmsError, "COS_A", "COS_A_ts_001", dataset );

	// READ VARIABLES
	MmsValue* dataSet;
	//dataSetValue = MmsConnection_readVariable(con, &mmsError, "COS_A", "ds_001_dig");
	
	dataSet = MmsConnection_readNamedVariableListValues(con, &mmsError, "COS_A", "ds_001_dig", 0);
	if (dataSet == NULL){
		printf("reading value failed! %d\n", mmsError);                                                                                                   
		return 0;
	}else{
		for (i=0; i < number_of_variables; i ++) {
		MmsValue* dataSetValue;
		MmsValue* dataSetElem;
		MmsValue* timeStamp;
		time_t time_stamp;
		struct tm * time_result;
		
		//GET DATASET VALUES (FIRST 3 VALUES ARE ACCESS-DENIED)
		dataSetValue = MmsValue_getElement(dataSet, 3+i);
		if(dataSetValue == NULL) {
			printf("could not get DATASET values\n");
			return 0;
		}

		//First element Data_TimeStampExtended
		dataSetElem = MmsValue_getElement(dataSetValue, 0);
		if(dataSetElem == NULL) {
			printf("could not get digital Data_TimeStampExtended\n");
			return 0;
		}
		timeStamp = MmsValue_getElement(dataSetElem, 0);

		if(timeStamp == NULL) {
			printf("could not get digital timestamp value\n");
			return 0;
		}
		time_stamp = MmsValue_toUint32(timeStamp);
		time_result = localtime(&time_stamp);
		printf("Data TimeStamp %s\n", asctime(time_result));

		//Second Element DataState
		dataSetElem = MmsValue_getElement(dataSetValue, 1);
		if(dataSetElem == NULL) {
			printf("could not get digital DataState\n");
			return 0;
		}
		handle_digital_state(dataSetElem);
 		MmsValue_delete(dataSetValue); 

		}
		MmsConnection_setInformationReportHandler(con, informationReportHandler, NULL);
#if 0
		/* include data set name in the report */
		MmsValue* optFlds = MmsValue_newBitString(10);
		MmsValue_setBitStringBit(optFlds, 4, true);
		MmsConnection_writeVariable(con, &mmsError, "ied1Inverter", "LLN0$RP$rcb1$OptFlds", optFlds);
		MmsValue_delete(optFlds);

		/* trigger only on data change or data update */
		MmsValue* trgOps = MmsValue_newBitString(6);
		MmsValue_setBitStringBit(trgOps, 1, true);
		MmsValue_setBitStringBit(trgOps, 3, true);
		MmsConnection_writeVariable(con, &mmsError, "ied1Inverter", "LLN0$RP$rcb1$TrgOps", optFlds);
		MmsValue_delete(trgOps);

		/* Enable report control block */
		MmsValue* rptEnable = MmsValue_newBoolean(true);
		MmsConnection_writeVariable(con, &mmsError, "ied1Inverter", "LLN0$RP$rcb1$RptEna", rptEnable);
		MmsValue_delete(rptEnable);
#endif

		MmsValue* loop_value;
		while(running) {
			loop_value = MmsConnection_readVariable(con, &mmsError, "COS_A", "Bilateral_Table_ID");
			if (loop_value == NULL){ 
				printf("reading value failed! %d\n", mmsError);                                                                                                   
				continue;
			}else{                                                                                                                                     
				sleep(1);
			}
		}

		MmsValue_delete(value); 
	}
	/*
	   value = MmsConnection_readVariable(con, &mmsError, "TESTE/MAR04", "C1E26ADB1A");
	   if (value == NULL)                                                                                                                       
	   printf("reading value failed! %d\n", mmsError);                                                                                                   
	   else{                                                                                                                                     
	   printf("Read variable with value: %f\n", MmsValue_toFloat(value));
	   MmsValue_delete(value); 
	   }
	 */
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
	}else {
		printf("erro de conexão %d %d", indication, mmsError);
	}

	MmsConnection_destroy(con);
	return 0;
}
