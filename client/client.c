#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include "mms_client_connection.h"

int main (int argc, char ** argv){

//	LinkedList nameList;
	MmsClientError mmsError;

	MmsConnection con = MmsConnection_create();
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
	if (value == NULL)                                                                                                                       
		printf("reading value failed! %d\n", mmsError);                                                                                                   
	else{                                                                                                                                     
		printf("Read variable with value: %f\n", MmsValue_toFloat(value));
		MmsValue_delete(value); 
	}


	//CREATE DATASETS
	LinkedList variables = LinkedList_create();
	MmsVariableSpecification * name = MmsVariableSpecification_create ("COS_A", "Transfer_Set_Name");
	MmsVariableSpecification * ts   = MmsVariableSpecification_create ("COS_A", "Transfer_Set_Time_Stamp");
	MmsVariableSpecification * ds   = MmsVariableSpecification_create ("COS_A", "DSConditions_Detected");
	MmsVariableSpecification * cbo  = MmsVariableSpecification_create ("COS_A", "CBO$AL6$$XCBR5233");
	cbo->arrayIndex = 0;	

	LinkedList_add(variables, name );
	LinkedList_add(variables, ts);
	LinkedList_add(variables, ds);
	LinkedList_add(variables, cbo);

	MmsConnection_defineNamedVariableList(con, &mmsError, "COS_A", "ds_001_dig", variables); 



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
		MmsValue* dataSetValue;
		MmsValue* dataSetElem;
		MmsValue* timeStamp;
		time_t time_stamp;
		struct tm * time_result;
		
		//GET DATASET VALUES (FIRST 3 VALUES ARE ACCESS-DENIED)
		dataSetValue = MmsValue_getElement(dataSet, 3);
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
		printf("TimeStamp %s\n", asctime(time_result));

		//Second Element DataState
		dataSetElem = MmsValue_getElement(dataSetValue, 1);
		if(dataSetElem == NULL) {
			printf("could not get digital DataState\n");
			return 0;
		}
		printf("State_hi %d State_lo %d, Validity_hi %d, Validity_lo %d, CurrentSource_hi %d, CurrentSource_lo %d, NormalValue %d, TimeStampQuality %d \n", 
				MmsValue_getBitStringBit(dataSetElem,0),
				MmsValue_getBitStringBit(dataSetElem,1),
				MmsValue_getBitStringBit(dataSetElem,2),
				MmsValue_getBitStringBit(dataSetElem,3),
				MmsValue_getBitStringBit(dataSetElem,4),
				MmsValue_getBitStringBit(dataSetElem,5),
				MmsValue_getBitStringBit(dataSetElem,6),
				MmsValue_getBitStringBit(dataSetElem,7) );

		if (MmsValue_getBitStringBit(dataSetElem,0) && !MmsValue_getBitStringBit(dataSetElem,1)) {
			printf("Estado Ligado\n");
		}
		if (!MmsValue_getBitStringBit(dataSetElem,0) && MmsValue_getBitStringBit(dataSetElem,1)) {
			printf("Estado Desligado\n");
		} else {
			printf ("Estado desconhecido\n");
		}

		if (!MmsValue_getBitStringBit(dataSetElem,0) && !MmsValue_getBitStringBit(dataSetElem,1)) {
			printf("Ponto Valido\n");
		}
		if (!MmsValue_getBitStringBit(dataSetElem,0) && MmsValue_getBitStringBit(dataSetElem,1)) {
			printf("Estado Desligado\n");
		} else {
			printf ("Estado desconhecido\n");
		}


 		MmsValue_delete(dataSetValue); 
	}
		LinkedList_destroy(variables);
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
