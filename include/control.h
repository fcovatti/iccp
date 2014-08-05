
#ifndef CONTROL_H_INCLUDED
#define CONTROL_H_INCLUDED

#include "mms_client_connection.h"

	bool
ControlObjectClient_select(char * objectReference, MmsConnection con);


bool
ControlObjectClient_operate(char * objectReference, MmsConnection con, MmsValue* ctlVal, int * ctlNum, char hasTimeActivatedMode, char test, char interlockCheck, char synchroCheck, uint64_t operTime);
#endif
