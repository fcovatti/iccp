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

		MmsValue* value;
 		value = MmsConnection_readVariable(con, &mmsError, "TESTE/MAR04", "C1E26ADB1A");
		if (value == NULL)                                                                                                                       
			printf("reading value failed! %d\n", mmsError);                                                                                                   
		else{                                                                                                                                     
			printf("Read variable with value: %f\n", MmsValue_toFloat(value));
	 		MmsValue_delete(value); 
		}
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
