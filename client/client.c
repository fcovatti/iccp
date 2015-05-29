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
//#define DEBUG_READ_DATASET 1
//#define DEBUG_DIGITAL_REPORTS 1
//#define DEBUG_EVENTS_REPORTS	1
//#define LOG_CONFIG	1
//#define LOG_COUNTERS	1
//#define DATA_LOG 1
//#define WINE_TESTING 1
/*********************************************************************************************************/

static int running = 1; //used on handler for signal interruption

/***************************************Configuration**************************************************/
// Variables used alongside the code to control the total of each element
static int num_of_analog_ids = 0;	
static int num_of_digital_ids = 0;	
static int num_of_event_ids = 0;	
static int num_of_commands = 0;	
static int num_of_datasets = 0;
static int num_of_analog_datasets = 0;
static int num_of_digital_datasets = 0;
static int num_of_event_datasets = 0;

//Configuration of the ICCP client
static data_config * analog_cfg = NULL;
static data_config * digital_cfg = NULL;
static data_config * events_cfg = NULL;
static command_config * commands = NULL;
static dataset_config * dataset_conf = NULL;

//ICCP Servers Main and Backup data
static st_server_data srv_main;
static st_server_data srv_bckp;

//Variables Configured on ICCP_CLIENT_CONFIG_FILE file
static char IDICCP[MAX_ID_ICCP_NAME];
static char srv1[MAX_STR_NAME], srv2[MAX_STR_NAME], srv3[MAX_STR_NAME], srv4[MAX_STR_NAME]; 
static char srv5[MAX_STR_NAME], srv6[MAX_STR_NAME], srv7[MAX_STR_NAME], srv8[MAX_STR_NAME]; 
static int integrity_time=0, analog_buf=0, digital_buf=0, events_buf=0;
static int convert_hyphen_to_dollar=0;

//Backup ICCP client variables
static char bkp_addr[MAX_STR_NAME];
static int bkp_socket;
static int bkp_enabled;
static int bkp_present;
static unsigned int bkp_signature = ICCP_BACKUP_SIGNATURE;
static struct sockaddr_in bkp_sock_addr;

//STATS ICCP variables
static char stats_addr[MAX_STR_NAME];
static int stats_socket_send;
static int stats_socket_receive;
static unsigned int stats_signature = ICCP_STATS_SIGNATURE;
static struct sockaddr_in stats_sock_addr;

//IHM useful varibless
static char ihm_addr[MAX_STR_NAME];
static int ihm_socket_send=0;
static int ihm_socket_receive=0;
static int ihm_enabled=0;
static int ihm_station=0;
static struct sockaddr_in ihm_sock_addr;
static struct timeval start, curr_time;

//IHM Counters
static unsigned int num_of_report_msgs;
static unsigned int num_of_digital_msgs;
static unsigned int num_of_analog_msgs;

//IHM Analog buffer
static st_analog_queue analog_queue;
static st_digital_queue digital_queue;
static Semaphore digital_mutex;

// Localtime mutex for not crashing under win32 platforms
Semaphore localtime_mutex;

//Threads
static Thread connections_thread;
static Thread bkp_thread;
static Thread stats_thread;


/*********************************************************************************************************/
#ifdef DATA_LOG
static FILE * data_file_analog = NULL;
static FILE * data_file_digital = NULL;
static FILE * data_file_events = NULL;
#endif

//Log file
FILE * log_file = NULL;

/*********************************************************************************************************/
static int get_time_ms(){
	gettimeofday(&curr_time, NULL);
	return ((curr_time.tv_sec-start.tv_sec)*1000 + (curr_time.tv_usec-start.tv_usec)/1000);
}
/*********************************************************************************************************/
void handle_analog_integrity(st_server_data *srv_data, int dataset, data_to_handle * handle)
{
	int i, offset, msg_queue =0;
	unsigned int npontos[MAX_MSGS_GI_ANALOG];
	float values[MAX_MSGS_GI_ANALOG];
	unsigned char states[MAX_MSGS_GI_ANALOG];
	memset(npontos, 0, MAX_MSGS_GI_ANALOG*sizeof(unsigned int));
	memset(values, 0, MAX_MSGS_GI_ANALOG*sizeof(float));
	memset(states, 0, MAX_MSGS_GI_ANALOG*sizeof(unsigned char));

	offset = dataset_conf[dataset].offset;
	for(i=0;i<dataset_conf[dataset].size;i++){
		if(!srv_data->analog[offset+i].not_present){
			srv_data->analog[offset+i].f = handle[i].f;
			srv_data->analog[offset+i].state = handle[i].state;

#ifdef HANDLE_ANALOG_DATA_DEBUG	
			printd("AI:%25s: %11.2f %-6s |", analog_cfg[offset+i].id, handle[i].f,analog_cfg[offset+i].state_on);
			print_value(handle[i].state,1, handle[i].time_stamp,0, "", "");
#endif

			analog_cfg[offset+i].num_of_msg_rcv++;
			npontos[msg_queue] = analog_cfg[offset+i].nponto;
			values[msg_queue]= handle[i].f;
			states[msg_queue]= handle[i].state;
			msg_queue++;
			if (!(msg_queue%MAX_MSGS_GI_ANALOG)){//manda a cada 125
				if(ihm_enabled && ihm_socket_send > 0){
					if(send_analog_list_to_ihm(ihm_socket_send, &ihm_sock_addr,npontos, ihm_station, values, states, MAX_MSGS_GI_ANALOG)< 0){
						LOG_MESSAGE( "Error sending list of analog ds %d part %d\n", dataset,msg_queue/MAX_MSGS_GI_ANALOG); 
					}else{
						num_of_analog_msgs++;
					}
					memset(npontos, 0, MAX_MSGS_GI_ANALOG);
					memset(values,  0, MAX_MSGS_GI_ANALOG);
					memset(states,  0, MAX_MSGS_GI_ANALOG);
				}
				msg_queue=0;
			}
		}
	}
	//if dataset size is not exactly multiple of queues...send what is left
	if(msg_queue%MAX_MSGS_GI_ANALOG && ihm_enabled && ihm_socket_send > 0){
		if(send_analog_list_to_ihm(ihm_socket_send, &ihm_sock_addr,npontos, ihm_station, values, states, (msg_queue%MAX_MSGS_GI_ANALOG))< 0){
			LOG_MESSAGE( "Error sending list of analog ds %d part %d\n", dataset,msg_queue/MAX_MSGS_GI_ANALOG); 
		}else{
			num_of_analog_msgs++;
		}
	}
}

/*********************************************************************************************************/
void handle_digital_integrity(st_server_data *srv_data, int dataset, data_to_handle * handle){
	int i, offset, msg_queue =0;
	offset = dataset_conf[dataset].offset;
	unsigned int npontos[MAX_MSGS_GI_DIGITAL];
	unsigned char states[MAX_MSGS_GI_DIGITAL];
	memset(npontos, 0, MAX_MSGS_GI_DIGITAL*sizeof(unsigned int));
	memset(states, 0, MAX_MSGS_GI_DIGITAL*sizeof(unsigned char));

	for(i=0;i<dataset_conf[dataset].size;i++){
		if(!srv_data->digital[offset+i].not_present){
			srv_data->digital[offset+i].time_stamp = handle[i].time_stamp;
			srv_data->digital[offset+i].time_stamp_extended = handle[i].time_stamp_extended;
			srv_data->digital[offset+i].state = handle[i].state;

#ifdef HANDLE_DIGITAL_DATA_DEBUG	
			printd("DI:%25s: ", digital_cfg[offset+i].id);
			print_value(handle[i].state,0, handle[i].time_stamp, handle[i].time_stamp_extended, digital_cfg[offset+i].state_on, digital_cfg[offset+i].state_off);
#endif

			digital_cfg[offset+i].num_of_msg_rcv++;
			npontos[msg_queue] = digital_cfg[offset+i].nponto;
			states[msg_queue] = handle[i].state;
			msg_queue++;
			if (!(msg_queue%MAX_MSGS_GI_DIGITAL)){//if queue is full
				if(ihm_enabled && ihm_socket_send > 0){
					if(send_digital_list_to_ihm(ihm_socket_send, &ihm_sock_addr,npontos, ihm_station, states, MAX_MSGS_GI_DIGITAL)< 0){
						LOG_MESSAGE( "Error sending list of digital ds %d part %d\n", dataset,msg_queue/MAX_MSGS_GI_DIGITAL); 					
					}else{
						num_of_digital_msgs++;
					}
					memset(npontos, 0, MAX_MSGS_GI_DIGITAL);
					memset(states,  0, MAX_MSGS_GI_DIGITAL);
				}
				msg_queue=0;
			}
		}
	}
	//if dataset size is not exactly multiple of queues...send what is left
	if(msg_queue%MAX_MSGS_GI_DIGITAL && ihm_enabled && ihm_socket_send > 0){
		if(send_digital_list_to_ihm(ihm_socket_send, &ihm_sock_addr,npontos, ihm_station, states,  (msg_queue%MAX_MSGS_GI_DIGITAL))< 0){
			LOG_MESSAGE( "Error sending list of digital ds %d part %d\n", dataset,msg_queue/MAX_MSGS_GI_DIGITAL); 
		}else{
			num_of_digital_msgs++;
		}
	}
}

/*********************************************************************************************************/
void handle_events_integrity(st_server_data *srv_data, int dataset, data_to_handle * handle){
	int i, offset,  msg_queue =0;
	offset = dataset_conf[dataset].offset;
	//just store first state for events	
	for(i=0;i<dataset_conf[dataset].size;i++){
		if(!srv_data->events[offset+i].not_present){
			srv_data->events[offset+i].time_stamp = handle[i].time_stamp;
			srv_data->events[offset+i].time_stamp_extended = handle[i].time_stamp_extended;
			srv_data->events[offset+i].state = handle[i].state;
#ifdef HANDLE_EVENTS_DATA_DEBUG	
			printd("EI:%25s: ", events_cfg[offset+i].id);
			print_value(handle[i].state,0, handle[i].time_stamp, handle[i].time_stamp_extended, events_cfg[offset+i].state_on, events_cfg[offset+i].state_off);
#endif

		}
	}
}
/*********************************************************************************************************/
void handle_analog_report(st_server_data *srv_data, float value, unsigned char state, unsigned int index,time_t time_stamp){

	if (analog_cfg != NULL && srv_data!=NULL && index >=0 && index < num_of_analog_ids) {

#ifdef DATA_LOG
		if((srv_data->analog[index]->f != value) || (srv_data->analog[index]->state != state)){
			data_analog_out data;
			data.nponto = analog_cfg[index].nponto;
			data.state = state;
			data.time_stamp = time_stamp;
			data.f = value;
			fwrite(&data,1,sizeof(data_analog_out),data_file_analog);
		}
#endif
		srv_data->analog[index].f = value;
		srv_data->analog[index].state = state;
		srv_data->analog[index].time_stamp = time_stamp;
		analog_cfg[index].num_of_msg_rcv++;
		analog_cfg[index].num_of_reports++;

#ifdef HANDLE_ANALOG_DATA_DEBUG	
		printd("AR:%25s: %11.2f %-6s |", analog_cfg[index].id, value,analog_cfg[index].state_on);
		print_value(state,1, time_stamp,0, "", "");
#endif

		//BUFFERING ANALOG DATA
		if(ihm_enabled && ihm_socket_send > 0){
			Semaphore_wait(analog_queue.mutex);	
			if(!analog_queue.size)
				analog_queue.time=get_time_ms();//first income packet store time of receive
			analog_queue.states[analog_queue.size]=state;
			analog_queue.values[analog_queue.size]=value;
			analog_queue.npontos[analog_queue.size]=analog_cfg[index].nponto;
			analog_queue.size++;
			if(analog_queue.size==MAX_MSGS_SQ_ANALOG){//if queue is full, send to ihm
				if(send_analog_list_to_ihm(ihm_socket_send, &ihm_sock_addr,analog_queue.npontos, ihm_station, analog_queue.values, analog_queue.states, MAX_MSGS_SQ_ANALOG) <0){
					LOG_MESSAGE( "Error sending analog buffer \n");
				}else{
					num_of_analog_msgs++;
					analog_queue.size=0;
				}
			}
			Semaphore_post(analog_queue.mutex);	
		}
	}
}
/*********************************************************************************************************/
void handle_digital_report(st_server_data *srv_data, unsigned char state, unsigned int index,time_t time_stamp, unsigned short time_stamp_extended){
	int flapping=0;
	if (digital_cfg != NULL && srv_data!= NULL && index >=0 && index < num_of_digital_ids) {

#ifdef DATA_LOG
		if((srv_data->digital[index].state != state)){
			data_digital_out data;
			data.nponto = digital_cfg[index].nponto;
			data.state = state;
			data.time_stamp = time_stamp;
			data.time_stamp_extended = time_stamp_extended;
			fwrite(&data,1,sizeof(data_digital_out),data_file_digital);
		}
#endif
		//if timestamp is invalid and last value is equal to new value  
		if((state&STATE_MASK_DATA_VALUE)==(srv_data->digital[index].state&STATE_MASK_DATA_VALUE)){
			if ((state&STATE_MASK_TIMESTAMP_INVALID)){
				flapping=1;
				digital_cfg[index].num_of_flapping++;
			}
		}

		digital_cfg[index].num_of_msg_rcv++;

		if(ihm_enabled && ihm_socket_send > 0){
			//if invalid timestamp buffer data
			//and if state is flapping
			if(flapping){
				//BUFFERING DIGITAL DATA
				Semaphore_wait(digital_queue.mutex);	
				if(!digital_queue.size)
					digital_queue.time=get_time_ms();//first income packet store time of receive
				digital_queue.states[digital_queue.size]=state;
				digital_queue.npontos[digital_queue.size]=digital_cfg[index].nponto;
				digital_queue.size++;
				if(digital_queue.size==MAX_MSGS_SQ_DIGITAL){//if queue is full, send to ihm
					if(send_digital_list_to_ihm(ihm_socket_send, &ihm_sock_addr,digital_queue.npontos, ihm_station, digital_queue.states, MAX_MSGS_SQ_DIGITAL) <0){
						LOG_MESSAGE( "Error sending digital buffer \n");
					}else{
						num_of_digital_msgs++;
						digital_queue.size=0;
					}
				}
				Semaphore_post(digital_queue.mutex);	
			}else{
				digital_cfg[index].num_of_reports++;
#ifdef DEBUG_DIGITAL_REPORTS	
				printd("DR%d:%25s: ", flapping, digital_cfg[index].id); //digital report
				print_value(state,0, time_stamp, time_stamp_extended, digital_cfg[index].state_on, digital_cfg[index].state_off);
				printd("                                ");
				print_value(srv_data->digital[index].state,0, srv_data->digital[index].time_stamp, srv_data->digital[index].time_stamp_extended, digital_cfg[index].state_on, digital_cfg[index].state_off);
#endif
				Semaphore_wait(digital_mutex);	
				if(send_digital_to_ihm(ihm_socket_send, &ihm_sock_addr, digital_cfg[index].nponto, ihm_station, state, time_stamp, time_stamp_extended, 1) < 0){
					LOG_MESSAGE( "Error sending nponto %d\n", digital_cfg[index].nponto);
				}else{
					num_of_report_msgs++;
				}
				Semaphore_post(digital_mutex);	
			}
		}
		srv_data->digital[index].state = state;
		srv_data->digital[index].time_stamp = time_stamp;
		srv_data->digital[index].time_stamp_extended = time_stamp_extended;

	}
}
/*********************************************************************************************************/
void handle_event_report(st_server_data *srv_data, unsigned char state, unsigned int index,time_t time_stamp, unsigned short time_stamp_extended){
	int flapping=0;
	if (events_cfg != NULL && srv_data!=NULL && index >=0 && index < num_of_event_ids) {

#ifdef DATA_LOG
		if((srv_data->events[index].state != state)){
			data_digital_out data;
			data.nponto = events_cfg[index].nponto;
			data.state = state;
			data.time_stamp = time_stamp;
			data.time_stamp_extended = time_stamp_extended;
			fwrite(&data,1,sizeof(data_digital_out),data_file_events);
		}
#endif
		//if current state is invalid and last value is equal to new value
		//if last state is invalid and last value is equal to new value
		if((state&STATE_MASK_DATA_VALUE)==(srv_data->events[index].state&STATE_MASK_DATA_VALUE)){
			if((state&STATE_MASK_DATA_INVALID)){
				flapping=1;
				events_cfg[index].num_of_flapping++;
			}	
			else if((srv_data->events[index].state&STATE_MASK_DATA_INVALID)){
				flapping=2;
				events_cfg[index].num_of_flapping++;
			}
		}

		srv_data->events[index].state = state;
		srv_data->events[index].time_stamp = time_stamp;
		srv_data->events[index].time_stamp_extended = time_stamp_extended;
		events_cfg[index].num_of_msg_rcv++;

		if(ihm_enabled && ihm_socket_send > 0){
			//if invalid timestamp buffer data
			if((state&STATE_MASK_TIMESTAMP_INVALID) && flapping){
				//BUFFERING DIGITAL DATA
				Semaphore_wait(digital_queue.mutex);	
				if(!digital_queue.size)
					digital_queue.time=get_time_ms();//first income packet store time of receive
				digital_queue.states[digital_queue.size]=state;
				digital_queue.npontos[digital_queue.size]=events_cfg[index].nponto;
				digital_queue.size++;
				if(digital_queue.size==MAX_MSGS_SQ_DIGITAL){//if queue is full, send to ihm
					if(send_digital_list_to_ihm(ihm_socket_send, &ihm_sock_addr,digital_queue.npontos, ihm_station, digital_queue.states, MAX_MSGS_SQ_DIGITAL) <0){
						LOG_MESSAGE( "Error sending digital buffer \n");
					}else{
						num_of_digital_msgs++;
						digital_queue.size=0;
					}
				}
				Semaphore_post(digital_queue.mutex);	
			}else{
				events_cfg[index].num_of_reports++;
#ifdef DEBUG_EVENTS_REPORTS	
				printd("ER%d:%25s: ", flapping, events_cfg[index].id); //event report
				print_value(state,0, time_stamp, time_stamp_extended, events_cfg[index].state_on, events_cfg[index].state_off);
#endif
				Semaphore_wait(digital_mutex);	
				if(send_digital_to_ihm(ihm_socket_send, &ihm_sock_addr, events_cfg[index].nponto, ihm_station, state, time_stamp, time_stamp_extended, 1) < 0){
					LOG_MESSAGE( "Error sending nponto %d\n", events_cfg[index].nponto);
				}else{
					num_of_report_msgs++;
				}
				Semaphore_post(digital_mutex);	
			}
		}
	}
}
/*********************************************************************************************************/
static int read_dataset(st_server_data * srv_data, char * ds_name, unsigned int offset){
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
	data_to_handle handle[DATASET_MAX_SIZE]; 
	memset(handle, 0, sizeof(data_to_handle)*DATASET_MAX_SIZE);

	dataSet = MmsConnection_readNamedVariableListValues(srv_data->con, &mmsError, IDICCP, ds_name, 0);
	if (dataSet == NULL){
		LOG_MESSAGE("ERROR - reading dataset failed! %d\n", mmsError);                                                                                                   
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
				printd("%X", debug_read[debug_i]);
			}
			printd("\n");
#endif			
			if(dataSetValue == NULL) {
				LOG_MESSAGE( "ERROR - could not get DATASET values offset %d element %d %d \n",offset, idx, number_of_variables);
				continue;
			}

			if(dataset_conf[offset].type == DATASET_ANALOG){
				MmsValue* analog_value;
				float analog_data = 0.0;
				data_state = 0;

				//First element Floating Point Value
				analog_value = MmsValue_getElement(dataSetValue, 0);
				if(analog_value == NULL) {
					LOG_MESSAGE( "WARN - could not get floating point value %s - nponto %d\n", analog_cfg[idx].id, analog_cfg[idx].nponto);
					srv_data->analog[idx].not_present=1;
				}else { 
					analog_data = MmsValue_toFloat(analog_value);
					//Second Element BitString data state
					analog_value = MmsValue_getElement(dataSetValue, 1);
					if(analog_value == NULL) {
						LOG_MESSAGE( "ERROR - could not get analog state %s - nponto %d\n", analog_cfg[idx].id, analog_cfg[idx].nponto);
					} else {
						data_state = analog_value->value.bitString.buf[0];
					}
					time( &time_stamp);
					handle[i].state=data_state;
					handle[i].f=analog_data;

				}
			} else if(dataset_conf[offset].type == DATASET_DIGITAL){
				data_state = 0;
				//First element Data_TimeStampExtended
				dataSetElem = MmsValue_getElement(dataSetValue, 0);
				if(dataSetElem == NULL) {
					LOG_MESSAGE( "WARN - could not get digital Data_TimeStampExtended %s - nponto %d\n", digital_cfg[idx].id, digital_cfg[idx].nponto);
					time(&time_stamp); //use system time stamp
					time_stamp_extended=0;
					srv_data->digital[idx].not_present=1;
				}else{
					ts_extended[1]= dataSetElem->value.octetString.buf[0];
					ts_extended[0]= dataSetElem->value.octetString.buf[1];
					timeStamp = MmsValue_getElement(dataSetElem, 0);
					if(timeStamp == NULL) {
						LOG_MESSAGE( "ERROR - could not get digital timestamp value %s - nponto %d\n", digital_cfg[idx].id, digital_cfg[idx].nponto);
					}else{
						time_stamp = MmsValue_toUint32(timeStamp);
					}
				
					//Second Element DataState
					dataSetElem = MmsValue_getElement(dataSetValue, 1);
					if(dataSetElem == NULL) {
						LOG_MESSAGE( "ERROR - could not get digital DataState %s - nponto %d\n", digital_cfg[idx].id, digital_cfg[idx].nponto);
					}else {
						data_state = dataSetElem->value.bitString.buf[0];
					}
					handle[i].state=data_state;
					handle[i].time_stamp=time_stamp;
					handle[i].time_stamp_extended=time_stamp_extended;

				}
			} else if(dataset_conf[offset].type == DATASET_EVENTS){
				data_state = 0;
				//First element Data_TimeStampExtended
				dataSetElem = MmsValue_getElement(dataSetValue, 0);
				if(dataSetElem == NULL) {
					LOG_MESSAGE( "WARN - could not get event Data_TimeStampExtended %s - nponto %d\n", events_cfg[idx].id, events_cfg[idx].nponto);
					time(&time_stamp); //use system time stamp
					time_stamp_extended=0;
					srv_data->events[idx].not_present=1;
				}else{
					ts_extended[1]= dataSetElem->value.octetString.buf[0];
					ts_extended[0]= dataSetElem->value.octetString.buf[1];
					
					timeStamp = MmsValue_getElement(dataSetElem, 0);

					if(timeStamp == NULL) {
						LOG_MESSAGE( "ERROR - could not get event timestamp value %s - nponto %d\n", events_cfg[idx].id, events_cfg[idx].nponto);
					}
					time_stamp = MmsValue_toUint32(timeStamp);
					
					//Second Element DataState
					dataSetElem = MmsValue_getElement(dataSetValue, 1);
					if(dataSetElem == NULL) {
						LOG_MESSAGE( "ERROR - could not get event DataState %s - nponto %d\n", events_cfg[idx].id, events_cfg[idx].nponto);
					}else {
						data_state = dataSetElem->value.bitString.buf[0];
					}
					handle[i].state=data_state;
					handle[i].time_stamp=time_stamp;
					handle[i].time_stamp_extended=time_stamp_extended;
				}
			} else{
				LOG_MESSAGE( "ERROR - unknown configuration type for dataset %d offset %d\n", offset, idx);
				return -1;
			}
		}

		if(dataset_conf[offset].type == DATASET_DIGITAL){
			handle_digital_integrity(srv_data,offset, handle);
		} else if(dataset_conf[offset].type == DATASET_EVENTS){
			handle_events_integrity(srv_data,offset, handle);
		} else if(dataset_conf[offset].type == DATASET_ANALOG){
			handle_analog_integrity(srv_data,offset, handle);
		} else {
			LOG_MESSAGE( "ERROR - wrong type of dataset %d on read \n", offset);
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
			var = MmsVariableAccessSpecification_create (IDICCP, analog_cfg[i].id);
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
			var = MmsVariableAccessSpecification_create (IDICCP, digital_cfg[i].id);
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
			var = MmsVariableAccessSpecification_create (IDICCP, events_cfg[i].id);
			var->arrayIndex = 0;	
			LinkedList_add(variables, var);
		}
		dataset_conf[offset].type = DATASET_EVENTS;
		dataset_conf[offset].size = i-first;
		dataset_conf[offset].offset = first;


	}else {
		LOG_MESSAGE("ERROR creating unknown dataset\n");
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
	data_to_handle handle[DATASET_MAX_SIZE]; 
	memset(handle, 0, sizeof(data_to_handle)*DATASET_MAX_SIZE);
	
	ts_extended = (char *)&time_stamp_extended;
	time(&time_stamp);
	if (value != NULL && attributes != NULL && attributesCount ==4 && parameter != NULL) {
        st_server_data * srv_data = (st_server_data *)parameter;
		LinkedList list_names	 = LinkedList_getNext(attributes);
		while (list_names != NULL) {
			char * attribute_name = (char *) list_names->data;
			if(attribute_name == NULL){
				i++;
				LOG_MESSAGE( "ERROR - received report with null atribute name\n");
				continue;
			}
			list_names = LinkedList_getNext(list_names);
			MmsValue * dataSetValue = MmsValue_getElement(value, i);
			if(dataSetValue == NULL){
				i++;
				LOG_MESSAGE( "ERROR - received report with null dataset\n");
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
						LOG_MESSAGE( "ERROR - Empty domain id on report\n");
					}
					ts_name = MmsValue_getElement(dataSetValue, 2);
					if(ts_name !=NULL) {
						transfer_set = MmsValue_toString(ts_name);
					} else {
						LOG_MESSAGE( "ERROR - Empty transfer set name on report\n");
					}
				} else {
					LOG_MESSAGE("ERROR - Empty data for transfer set report\n");
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
							   printd(" %x", dataSetValue->value.octetString.buf[j]);
							   }
							   printd("\n"); */

							//RULE 0
							//first byte is the rule
							if(dataSetValue->value.octetString.buf[0] == 0) {
							
								if(dataSetValue->value.octetString.size == 0) {
									LOG_MESSAGE( "ERROR - empty octetString\n");
									return;
								}else if(dataSetValue->value.octetString.size < 2) {
									LOG_MESSAGE( "ERROR - no index in the octed string\n");
									return;
								}

								octet_offset = 1; 
								index = 0;
								dataset_conf[offset].num_of_rcv_gi++;

								while (octet_offset < dataSetValue->value.octetString.size){
									if(dataset_conf[offset].type == DATASET_DIGITAL){
										if(!srv_data->digital[index+dataset_conf[offset].offset].not_present){//In order to not threat non existent objects
											if( octet_offset+ RULE0_DIGITAL_REPORT_SIZE <= dataSetValue->value.octetString.size) {
												time_data time_value;
												time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
												time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
												time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
												time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
												ts_extended[1]= dataSetValue->value.octetString.buf[octet_offset+4];
												ts_extended[0]= dataSetValue->value.octetString.buf[octet_offset+5];
												handle[index].state=dataSetValue->value.octetString.buf[octet_offset+6];
												handle[index].time_stamp=time_value.t;
												handle[index].time_stamp_extended=time_stamp_extended;
											} else {
												LOG_MESSAGE( "ERROR - wrong digital report octet size %d, offset %d - data %d bytes - ds %d, idx %d \n",dataSetValue->value.octetString.size, octet_offset, RULE0_DIGITAL_REPORT_SIZE, offset, index );
											}
										}
										octet_offset = octet_offset + RULE0_DIGITAL_REPORT_SIZE;
									} else if(dataset_conf[offset].type == DATASET_EVENTS){
										if(!srv_data->events[index+dataset_conf[offset].offset].not_present){ //In order to not threat non existent objects
											if( octet_offset+RULE0_DIGITAL_REPORT_SIZE <= dataSetValue->value.octetString.size){
												time_data time_value;
												time_value.s[3] = dataSetValue->value.octetString.buf[octet_offset];
												time_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+1];
												time_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+2];
												time_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+3];
												ts_extended[1]= dataSetValue->value.octetString.buf[octet_offset+4];
												ts_extended[0]= dataSetValue->value.octetString.buf[octet_offset+5];
												handle[index].state=dataSetValue->value.octetString.buf[octet_offset+6];
												handle[index].time_stamp=time_value.t;
												handle[index].time_stamp_extended=time_stamp_extended;

											} else {
												LOG_MESSAGE( "ERROR - wrong event report octet size %d, offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE0_DIGITAL_REPORT_SIZE, offset, index);
											}
										}
										octet_offset = octet_offset + RULE0_DIGITAL_REPORT_SIZE;
									} else if(dataset_conf[offset].type == DATASET_ANALOG){
										offset_size=RULE0_ANALOG_REPORT_SIZE;
										if(srv_data->analog[index+dataset_conf[offset].offset].not_present){
											if(octet_offset+6 <= dataSetValue->value.octetString.size){ 
												//if not 0 invalid and last byte
												if(!dataSetValue->value.octetString.buf[octet_offset] && !dataSetValue->value.octetString.buf[octet_offset+1] &&
															!dataSetValue->value.octetString.buf[octet_offset+2] && !dataSetValue->value.octetString.buf[octet_offset+3] && dataSetValue->value.octetString.buf[octet_offset+4] == 0x30){
														offset_size=RULE0_ANALOG_REPORT_SIZE;
												}else if	(dataSetValue->value.octetString.buf[octet_offset+6] == 0x71){
													offset_size=RULE0_DIGITAL_REPORT_SIZE;//FIXME: for analog non existent objects the size is 7 and not threated when 0x53F3xxyy000071
												}else{
													LOG_MESSAGE("not present %d - %x %x %x %x %x %x %x\n",analog_cfg[index+dataset_conf[offset].offset].nponto
															,dataSetValue->value.octetString.buf[octet_offset+0]
															,dataSetValue->value.octetString.buf[octet_offset+1]
															,dataSetValue->value.octetString.buf[octet_offset+2]
															,dataSetValue->value.octetString.buf[octet_offset+3]
															,dataSetValue->value.octetString.buf[octet_offset+4]
															,dataSetValue->value.octetString.buf[octet_offset+5]
															,dataSetValue->value.octetString.buf[octet_offset+6]
														  );
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
												handle[index].state=dataSetValue->value.octetString.buf[octet_offset+4];
												handle[index].f=data_value.f;
											} else {
												LOG_MESSAGE( "ERROR - wrong analog report octet size %d, offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE0_ANALOG_REPORT_SIZE, offset, index);
											}
										}
										octet_offset = octet_offset + offset_size;
									}  else {
										LOG_MESSAGE( "ERROR - wrong index %d\n", offset);
										return;
									}
									index++;
								}
								// After put data on a list, treat them
								if(dataset_conf[offset].type == DATASET_DIGITAL){
									handle_digital_integrity(srv_data,offset, handle);
								} else if(dataset_conf[offset].type == DATASET_EVENTS){
									//handle_events_integrity(srv_data,offset, handle);//FIXME:do something else with GI events
									LOG_MESSAGE( "ERROR - events integrity are disabled dataset %d rule 0 \n", offset);
								} else if(dataset_conf[offset].type == DATASET_ANALOG){
									handle_analog_integrity(srv_data,offset, handle);
								} else {
									LOG_MESSAGE( "ERROR - wrong type of dataset %d on rule 0 \n", offset);
								}

							}else if(dataSetValue->value.octetString.buf[0] == 1) {
								//RULE 1 - not implemented
								LOG_MESSAGE( "WARNING - Information Report with rule 1 received\n");
								return;
							}
							else if (dataSetValue->value.octetString.buf[0] == 2) {
								//RULE 2
								if(dataSetValue->value.octetString.size == 0) {
									LOG_MESSAGE("ERROR - empty octetString\n");
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
											handle_digital_report(srv_data,dataSetValue->value.octetString.buf[octet_offset+RULE2_DIGITAL_REPORT_SIZE-1], index, time_value.t, time_stamp_extended);
										}else {
											LOG_MESSAGE("ERROR - Wrong size of digital report %d, octet_offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE2_DIGITAL_REPORT_SIZE, offset, index );
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
											handle_event_report(srv_data,dataSetValue->value.octetString.buf[octet_offset+RULE2_DIGITAL_REPORT_SIZE-1], index, time_value.t, time_stamp_extended);
										}else{
											LOG_MESSAGE("ERROR - Wrong size of event report %d, octet_offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE2_DIGITAL_REPORT_SIZE, offset, index  );
										}
										octet_offset = octet_offset + RULE2_DIGITAL_REPORT_SIZE;
									} else if(dataset_conf[offset].type == DATASET_ANALOG){
										if((RULE2_ANALOG_REPORT_SIZE + octet_offset)<= dataSetValue->value.octetString.size ) {
											float_data data_value;
											data_value.s[3] = dataSetValue->value.octetString.buf[octet_offset+2];
											data_value.s[2] = dataSetValue->value.octetString.buf[octet_offset+3];
											data_value.s[1] = dataSetValue->value.octetString.buf[octet_offset+4];
											data_value.s[0] = dataSetValue->value.octetString.buf[octet_offset+5];
											handle_analog_report(srv_data,data_value.f, dataSetValue->value.octetString.buf[octet_offset+6], index, time_stamp);
										}else {
											LOG_MESSAGE("ERROR - Wrong size of analog report %d, octet_offset %d - data %d bytes - ds %d, idx %d\n",dataSetValue->value.octetString.size, octet_offset, RULE2_ANALOG_REPORT_SIZE, offset, index  );
										}
										octet_offset = octet_offset + RULE2_ANALOG_REPORT_SIZE;
									} else {
										LOG_MESSAGE("ERROR - unkonwn information report index %d\n", index);
										MmsValue_delete(value);
										return;
									}
								}
							} else {
								LOG_MESSAGE("ERROR - invalid RULE\n");
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
							LOG_MESSAGE("ERROR - empty bitstring\n");
						}
					} else {
						LOG_MESSAGE("ERROR - NULL Element\n");
					}
				}
			}
			i++;
		}
		//CONFIRM RECEPTION OF EVENT
		MmsConnection_sendUnconfirmedPDU(srv_data->con,&mmsError,domain_id, transfer_set, time_stamp);
		MmsValue_delete(value);
	} else{
		LOG_MESSAGE( "ERROR - wrong report %d %d %d %d\n",value != NULL, attributes != NULL , attributesCount , parameter != NULL);
	}
}
/*********************************************************************************************************/
static int open_log_file(){
	/*****************
	 * OPEN LOG FILES
	 * **********/
	time_t t = time(NULL);
	struct tm now = *localtime(&t); 
	char flog[MAX_STR_NAME];

#ifdef WIN32
#ifdef WINE_TESTING
	snprintf(flog,MAX_STR_NAME, "/tmp/iccp_info-%04d%02d%02d%02d%02d.log", now.tm_year+1900, now.tm_mon+1, now.tm_mday,now.tm_hour, now.tm_min);
#else
	snprintf(flog,MAX_STR_NAME, "..\\logs\\iccp_info-%04d%02d%02d%02d%02d.log", now.tm_year+1900, now.tm_mon+1, now.tm_mday,now.tm_hour, now.tm_min);
#endif
#else
	snprintf(flog,MAX_STR_NAME, "/tmp/iccp_info-%04d%02d%02d%02d%02d.log", now.tm_year+1900, now.tm_mon+1, now.tm_mday,now.tm_hour, now.tm_min);
#endif

	log_file = fopen(flog, "w");
	if(log_file==NULL){
		printf("Error, cannot open log file %s\n",flog);
		fclose(log_file);
		return -1;
	}
	return 0;
}
/*********************************************************************************************************/
static int read_configuration() {
	FILE * file = NULL;
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
	int i;
	const char *str1;
	char id_iccp[MAX_STR_NAME], cfg_file[MAX_STR_NAME];
	char config_param[MAX_STR_NAME], config_value[MAX_STR_NAME];
	int cfg_params = 0;


	/*****************
	 * READ ICCP CONFIGURATION PARAMETERS
	 **********/
	file = fopen(ICCP_CLIENT_CONFIG_FILE, "r");
	if(file==NULL){
		LOG_MESSAGE( "WARN -  cannot open configuration file %s\n", ICCP_CLIENT_CONFIG_FILE);
#ifdef WIN32
		char cfg_file2[MAX_STR_NAME]; 
		snprintf(cfg_file2, MAX_STR_NAME, "..\\conf\\%s", ICCP_CLIENT_CONFIG_FILE);
		file = fopen(cfg_file2, "r");
		if (file==NULL){
			LOG_MESSAGE( "ERROR -  cannot open configuration file %s\n", cfg_file2);
			return -1;
		}
#else
		return -1;
#endif
	}
	
	while ( fgets(line, 300, file)){
		if (line[0] == '/' && line[1]=='/')
			continue;
		if(sscanf(line, "%[^=]=\"%[^\";]; ",config_param, config_value) < 1)
			break;
		if(strcmp(config_param, "IDICCP") == 0){
			snprintf(IDICCP, MAX_ID_ICCP_NAME, "%s", config_value);
			LOG_MESSAGE("IDICCP=%s\n", IDICCP);
			cfg_params++;
		}
		if(strcmp(config_param, "SERVER_NAME_1") == 0){
			snprintf(srv1, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("SERVER_NAME_1=%s\n", srv1);
			cfg_params++;
		}
		if(strcmp(config_param, "SERVER_NAME_2") == 0){
			snprintf(srv2, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("SERVER_NAME_2=%s\n", srv2);
			cfg_params++;
		}
		if(strcmp(config_param, "SERVER_NAME_3") == 0){
			snprintf(srv3, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("SERVER_NAME_3=%s\n", srv3);
			cfg_params++;
		}
		if(strcmp(config_param, "SERVER_NAME_4") == 0){
			snprintf(srv4, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("SERVER_NAME_4=%s\n", srv4);
			cfg_params++;
		}
		if(strcmp(config_param, "SERVER_NAME_5") == 0){
			snprintf(srv5, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("SERVER_NAME_5=%s\n", srv5);
			cfg_params++;
		}
		if(strcmp(config_param, "SERVER_NAME_6") == 0){
			snprintf(srv6, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("SERVER_NAME_6=%s\n", srv6);
			cfg_params++;
		}
		if(strcmp(config_param, "SERVER_NAME_7") == 0){
			snprintf(srv7, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("SERVER_NAME_7=%s\n", srv7);
			cfg_params++;
		}
		if(strcmp(config_param, "SERVER_NAME_8") == 0){
			snprintf(srv8, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("SERVER_NAME_8=%s\n", srv8);
			cfg_params++;
		}

		if(strcmp(config_param, "IHM_ADDRESS") == 0){
			snprintf(ihm_addr, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("IHM_ADDRESS=%s\n", ihm_addr);
			cfg_params++;
		}

		if(strcmp(config_param, "ICCP_BKP_ADDRESS") == 0){
			snprintf(bkp_addr, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("ICCP_BKP_ADDRESS=%s\n", bkp_addr);
			cfg_params++;
		}

		if(strcmp(config_param, "ICCP_STATS_ADDRESS") == 0){
			snprintf(stats_addr, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("ICCP_STATS_ADDRESS=%s\n", stats_addr);
			cfg_params++;
		}

		if(strcmp(config_param, "CONFIG_FILE") == 0){
			snprintf(cfg_file, MAX_STR_NAME, "%s", config_value);
			LOG_MESSAGE("CONFIG_FILE=%s\n", cfg_file);
			cfg_params++;
		}
		if(strcmp(config_param, "DATASET_INTEGRITY_TIME") == 0){
			integrity_time = atoi(config_value);
			LOG_MESSAGE("DATASET_INTEGRITY_TIME=%d\n", integrity_time);
			cfg_params++;
		}
		if(strcmp(config_param, "DATASET_ANALOG_BUFFER_INTERVAL") == 0){
			analog_buf = atoi(config_value);
			LOG_MESSAGE("DATASET_ANALOG_BUFFER_INTERVAL=%d\n", analog_buf);
			cfg_params++;
		}
		if(strcmp(config_param, "DATASET_DIGITAL_BUFFER_INTERVAL") == 0){
			digital_buf = atoi(config_value);
			LOG_MESSAGE("DATASET_DIGITAL_BUFFER_INTERVAL=%d\n", digital_buf);
			cfg_params++;
		}
		if(strcmp(config_param, "DATASET_EVENTS_BUFFER_INTERVAL") == 0){
			events_buf = atoi(config_value);
			LOG_MESSAGE("DATASET_EVENTS_BUFFER_INTERVAL=%d\n", events_buf);
			cfg_params++;
		}
		if(strcmp(config_param, "CONVERT_HYPHEN_TO_DOLLARSIGN") == 0){
			if (strcmp(config_value, "true") == 0){
				convert_hyphen_to_dollar = 1;
			} else if (strcmp(config_value, "false") == 0){
				convert_hyphen_to_dollar = 0;
			} else {
				LOG_MESSAGE( "ERROR - wrong parameters on %s\n",config_param);
				return -1;
			}
			LOG_MESSAGE("CONVERT_HYPHEN_TO_DOLLARSIGN=%d\n", config_value);
			cfg_params++;
		}
	}

	if (cfg_params!=18){
		LOG_MESSAGE( "ERROR - wrong number of parameters on %s\n",ICCP_CLIENT_CONFIG_FILE);
		return -1;
	}

	/*****************
	 * START CONFIGURATIONS
	 **********/

    analog_cfg = calloc(DATASET_MAX_SIZE*DATASET_ANALOG_MAX_NUMBER, sizeof(data_config) ); 
    digital_cfg = calloc(DATASET_MAX_SIZE*DATASET_DIGITAL_MAX_NUMBER, sizeof(data_config) ); 
    events_cfg  = calloc(DATASET_MAX_SIZE*DATASET_EVENTS_MAX_NUMBER, sizeof(data_config) ); 
	srv_main.analog = calloc(DATASET_MAX_SIZE*DATASET_ANALOG_MAX_NUMBER,   sizeof(data_to_handle) );
	srv_main.digital = calloc(DATASET_MAX_SIZE*DATASET_DIGITAL_MAX_NUMBER, sizeof(data_to_handle) );
	srv_main.events  = calloc(DATASET_MAX_SIZE*DATASET_EVENTS_MAX_NUMBER,  sizeof(data_to_handle) );
	srv_bckp.analog = calloc(DATASET_MAX_SIZE*DATASET_ANALOG_MAX_NUMBER,   sizeof(data_to_handle) );
	srv_bckp.digital = calloc(DATASET_MAX_SIZE*DATASET_DIGITAL_MAX_NUMBER, sizeof(data_to_handle) );
	srv_bckp.events  = calloc(DATASET_MAX_SIZE*DATASET_EVENTS_MAX_NUMBER,  sizeof(data_to_handle) ); 

    commands  = calloc(COMMANDS_MAX_NUMBER , sizeof(command_config) ); 

	file = fopen(cfg_file, "r");
	if(file==NULL){
		LOG_MESSAGE( "WARN - cannot open configuration file %s\n", cfg_file);
#ifdef WIN32
		char cfg_file2[MAX_STR_NAME]; 
		snprintf(cfg_file2, MAX_STR_NAME, "..\\conf\\%s", cfg_file);
		file = fopen(cfg_file2, "r");
		if (file==NULL){
			LOG_MESSAGE( "WARN - cannot open configuration file %s\n", cfg_file2);
			snprintf(cfg_file2, MAX_STR_NAME, "..\\conf\\point_list.txt");
			file = fopen(cfg_file2, "r");
			if (file==NULL){
				LOG_MESSAGE( "ERROR - cannot open configuration file %s\n", cfg_file2);
				return -1;
			}
		}
#else
		return -1;
#endif
	}
	
	//first two rows of CONFIG_FILE are the reader, discard them
	if(!fgets(line, 300, file)){
		LOG_MESSAGE( "ERROR - Error reading %s file header\n", cfg_file);
		return -1;
	}else {
		if(sscanf(line, "%*s %*d %*s %d", &ihm_station) <1){
			LOG_MESSAGE( "ERROR - cannot get ihm station from header\n");
			return -1;
		}else {
			LOG_MESSAGE("IHM station %d\n", ihm_station);
		}
	}

	if(!fgets(line, 300, file)){
		LOG_MESSAGE( "ERROR - Error reading %s file header second line\n", cfg_file);
		return -1;
	}

	while ( fgets(line, 300, file)){
		//if(sscanf(line, "%d %*d %22s %c", &configuration[num_of_ids].nponto,  configuration[num_of_ids].id, &configuration[num_of_ids].type ) <1)
		if(sscanf(line, "%d %*d %22s %c %31s %*d %*d %*d %d %*c %*d %*d %*f %*f %d %d", &nponto,  id_ponto, &type, state_name, &origin, &nponto_monitored, &event ) <1)
			break;

		//change - for $
		if (convert_hyphen_to_dollar){
			for ( i=0; i <22; i++) {
				if (id_ponto[i] == '-'){
					id_ponto[i] = '$';
				}
			}
		}
		//change + for $
		for ( i=0; i <22; i++) {
			if (id_ponto[i] == '+'){
				id_ponto[i] = '$';
			}
		}

	    if(origin == ORIGIN_CALC){
			LOG_MESSAGE("INFO - Ignoring Calculate object %s\n", id_ponto);
		}else if (origin == ORIGIN_MANUAL){
			LOG_MESSAGE("INFO - Ignoring Manual object %s\n", id_ponto);
		}//Command Digital or Analog
		else if((type == 'D'||type=='A') && origin == ORIGIN_COMMAND ){
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
			memcpy(events_cfg[num_of_event_ids].id,id_ponto,25);
			events_cfg[num_of_event_ids].nponto = nponto;
			
			state_split=0;
			for ( i=0; i <MAX_STR_NAME; i++) {
				if (state_name[i] == '/' ){
					state_split=i;
					events_cfg[num_of_event_ids].state_on[i]=0;
					continue;
				}
				if(state_split){
					if (state_name[i] == 0 ) {
						events_cfg[num_of_event_ids].state_off[i-state_split-1]=0;
						break;
					}else
						events_cfg[num_of_event_ids].state_off[i-state_split-1] = state_name[i];

				}else
					events_cfg[num_of_event_ids].state_on[i]=state_name[i];
			}
			num_of_event_ids++;
		} //Digital
		else if(type == 'D'){
			memcpy(digital_cfg[num_of_digital_ids].id,id_ponto,25);
			digital_cfg[num_of_digital_ids].nponto = nponto;
			
			state_split=0;
			for ( i=0; i <MAX_STR_NAME; i++) {
				if (state_name[i] == '/' ){
					state_split=i;
					digital_cfg[num_of_digital_ids].state_on[i]=0;
					continue;
				}
				if(state_split){
					if (state_name[i] == 0 ) {
						digital_cfg[num_of_digital_ids].state_off[i-state_split-1] =0;
						break;
					}else
						digital_cfg[num_of_digital_ids].state_off[i-state_split-1] = state_name[i];
				}else
					digital_cfg[num_of_digital_ids].state_on[i]=state_name[i];
			}
			num_of_digital_ids++;
		} //Analog
		else if(type == 'A'){
			memcpy(analog_cfg[num_of_analog_ids].id,id_ponto,25);
			analog_cfg[num_of_analog_ids].nponto = nponto;
			memcpy(analog_cfg[num_of_analog_ids].state_on,state_name,  16);
			num_of_analog_ids++;
		} //Unknown 
		else {
			LOG_MESSAGE("WARNING - ERROR reading configuration file! Unknown type\n");
		}
	}

#ifdef LOG_CONFIG
	LOG_MESSAGE( "***************ANALOG***********************\n");
	LOG_MESSAGE( " DATASET OFFSET NPONTO DESCRITIVO\n");
	for (i=0; i < num_of_analog_ids; i++) {
		LOG_MESSAGE( "%d %d %d \t%s\t \n",((i+1)/DATASET_MAX_SIZE), i, analog_cfg[i].nponto,  analog_cfg[i].id);
	}

	LOG_MESSAGE( "***************DIGITAL***********************\n");
	LOG_MESSAGE( " DATASET OFFSET NPONTO DESCRITIVO\n");
	for (i=0; i < num_of_digital_ids; i++) {
		LOG_MESSAGE( "%d %d %d \t%s\t \n",((i+1)/DATASET_MAX_SIZE),i, digital_cfg[i].nponto,  digital_cfg[i].id);
	}

	LOG_MESSAGE( "***************EVENTS***********************\n");
	LOG_MESSAGE( " DATASET OFFSET NPONTO DESCRITIVO\n");
	for (i=0; i < num_of_event_ids; i++) {
		LOG_MESSAGE( "%d %d %d \t%s\t \n",((i+1)/DATASET_MAX_SIZE),i, events_cfg[i].nponto,  events_cfg[i].id);
	}

	LOG_MESSAGE( "***************COMMANDS***********************\n");
	LOG_MESSAGE( " OFFSET NPONTO SUPERV DESCRITIVO \n");
	for (i=0; i < num_of_commands; i++) {
		LOG_MESSAGE( "%d %d \t %d \t%s \t\n",i, commands[i].nponto, commands[i].monitored,  commands[i].id);
	}
#endif

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
	return 0;
}
/*********************************************************************************************************/
static void sigint_handler(int signalId)
{
	running = 0;
}
/*********************************************************************************************************/
static void cleanup_variables()
{
	int i;
	MmsError mmsError;

	Thread_destroy(connections_thread);
	if(bkp_enabled)
		Thread_destroy(bkp_thread);

	if(srv_main.enabled)
		MmsConnection_conclude(srv_main.con, &mmsError);

	if(srv_bckp.enabled)
		MmsConnection_conclude(srv_bckp.con, &mmsError);

	MmsConnection_destroy(srv_main.con);
	MmsConnection_destroy(srv_bckp.con);

	LOG_MESSAGE("cleanning up variables\n");

#ifdef LOG_COUNTERS
	LOG_MESSAGE( "***************ANALOG***********************\n");
	LOG_MESSAGE( " DATASET OFFSET NPONTO RCV_MSGS DESCRITIVO\n");
	for (i=0; i < num_of_analog_ids; i++) {
		LOG_MESSAGE( "%7d %6d %6d %7d \t%s\t \n",((i+1)/DATASET_MAX_SIZE),i, analog_cfg[i].nponto, analog_cfg[i].num_of_msg_rcv, analog_cfg[i].id);
	}

	LOG_MESSAGE( "***************DIGITAL***********************\n");
	LOG_MESSAGE( " DATASET OFFSET NPONTO RCV_MSGS DESCRITIVO\n");
	for (i=0; i < num_of_digital_ids; i++) {
		LOG_MESSAGE( "%7d %6d %6d %7d \t%s\t \n",((i+1)/DATASET_MAX_SIZE),i, digital_cfg[i].nponto, digital_cfg[i].num_of_msg_rcv, digital_cfg[i].id);
	}

	LOG_MESSAGE( "***************EVENTS***********************\n");
	LOG_MESSAGE( " DATASET OFFSET NPONTO RCV_MSGS DESCRITIVO\n");
	for (i=0; i < num_of_event_ids; i++) {
		LOG_MESSAGE( "%7d %6d %6d %7d \t%s\t \n",((i+1)/DATASET_MAX_SIZE),i, events_cfg[i].nponto, events_cfg[i].num_of_msg_rcv, events_cfg[i].id);
	}
	LOG_MESSAGE("Total Sent %d - A:%d D:%d R:%d\n", (num_of_digital_msgs+num_of_analog_msgs+num_of_report_msgs),
				 num_of_analog_msgs, num_of_digital_msgs, num_of_report_msgs);
#endif
#ifdef DATA_LOG
	fclose(data_file_analog);
	fclose(data_file_digital);
	fclose(data_file_events);
#endif
	fclose(log_file);
	free(dataset_conf);
	free(analog_cfg);
	free(digital_cfg);
	free(events_cfg);
	free(srv_main.analog);
	free(srv_main.digital);
	free(srv_main.events);
	free(srv_bckp.analog);
	free(srv_bckp.digital);
	free(srv_bckp.events);

	if(ihm_enabled){
		close(ihm_socket_send);
		close(ihm_socket_receive);
	}
	if(bkp_enabled){
		close(bkp_socket);
	}

	close(stats_socket_send);
	close(stats_socket_receive);

}
/*********************************************************************************************************/
#ifdef DATA_LOG
static int open_data_logs(void) {
	data_file_analog = fopen(DATA_ANALOG_LOG, "w");
	if(data_file_analog==NULL){
		LOG_MESSAGE("Error, cannot open configuration data log file %s\n", DATA_ANALOG_LOG);
		return -1;
	}
	data_file_digital = fopen(DATA_DIGITAL_LOG, "w");
	if(data_file_digital==NULL){
		LOG_MESSAGE("Error, cannot open configuration data log file %s\n", DATA_DIGITAL_LOG);
		return -1;
	}
	data_file_events = fopen(DATA_EVENTS_LOG, "w");
	if(data_file_events==NULL){
		LOG_MESSAGE("Error, cannot open configuration data log file %s\n", DATA_EVENTS_LOG);
		return -1;
	}
	return 0;

}
#endif
/*********************************************************************************************************/
static int create_ihm_comm(){
	ihm_socket_send = prepare_Send(ihm_addr, PORT_IHM_TRANSMIT, &ihm_sock_addr);
	if(ihm_socket_send < 0){
		LOG_MESSAGE("could not create UDP socket to trasmit to IHM\n");
		return -1;
	}	
	LOG_MESSAGE("Created UDP socket %d for IHM %s Port %d\n",ihm_socket_send, ihm_addr, PORT_IHM_TRANSMIT);

	ihm_socket_receive = prepare_Wait(PORT_IHM_LISTEN);
	if(ihm_socket_receive < 0){
		LOG_MESSAGE("could not create UDP socket to listen to IHM\n");
		close(ihm_socket_send);
		return -1;
	}
	LOG_MESSAGE("Created UDP local socket %d for IHM Port %d\n",ihm_socket_receive,PORT_IHM_LISTEN);
	return 0;
}
/*********************************************************************************************************/
static int create_bkp_comm(){

	bkp_socket = prepare_Wait(PORT_ICCP_BACKUP);
	if(bkp_socket < 0){
		LOG_MESSAGE("could not create UDP socket to listen to Backup ICCP Client\n");
		return -1;
	}
	if(prepareServerAddress(bkp_addr, PORT_ICCP_BACKUP, &bkp_sock_addr) < 0){
	  	LOG_MESSAGE("error preparing ICCP backup address\n");
	  	return -1;
	}
	LOG_MESSAGE("Created UDP socket %d for ICCP Bakcup Port %d\n",bkp_socket,PORT_ICCP_BACKUP);
	return 0;
}
/*********************************************************************************************************/
static int create_stats_comm(){

	stats_socket_receive = prepare_Wait(PORT_STATS_LISTEN);
	if(stats_socket_receive < 0){
		LOG_MESSAGE("could not create UDP socket to listen to Stats Messages\n");
		return -1;
	}
	LOG_MESSAGE("Created UDP socket %d for stats listen Port %d\n",stats_socket_receive,PORT_STATS_LISTEN);
	
	stats_socket_send = prepare_Send(stats_addr, PORT_STATS_TRANSMIT, &stats_sock_addr);
	if(stats_socket_send < 0){
		LOG_MESSAGE("could not create UDP socket to trasmit stats\n");
		close(stats_socket_receive);
		return -1;
	}	
	
	LOG_MESSAGE("Created UDP socket %d for stats transmit Port %d\n",stats_socket_send,PORT_STATS_TRANSMIT);
	return 0;
}

/*********************************************************************************************************/
static int check_backup(unsigned int msg_timeout){
	char * msg_rcv;
	msg_rcv = WaitT(bkp_socket, msg_timeout);	
	if(msg_rcv != NULL) {
		unsigned int msg_code = 0;
		memcpy(&msg_code, msg_rcv, sizeof(unsigned int));
		free(msg_rcv);
		if(msg_code == ICCP_BACKUP_SIGNATURE) 
			return 0;		
	}
	return -1;
}
/*********************************************************************************************************/
static void check_commands(){
	char * msg_rcv;
	msg_rcv = WaitT(ihm_socket_receive, 2000);	
	if(msg_rcv != NULL) {
		int i;
		t_msgcmd cmd_recv = {0};
		memcpy(&cmd_recv, msg_rcv, sizeof(t_msgcmd));
		if(cmd_recv.signature != 0x4b4b4b4b) {
			char * cmd_debug = (char *)&cmd_recv;
			LOG_MESSAGE("ERROR - command received with wrong signature!\n");  
			for (i=0; i < sizeof(t_msgcmd); i++){
				LOG_MESSAGE(" %02x", cmd_debug[i]);
			}
			LOG_MESSAGE("\n");
		}
		for (i=0; i < num_of_commands; i++) {
			if(cmd_recv.endereco==commands[i].nponto)
				break;
		}
		if(i<num_of_commands){
			commands[i].num_of_msg_rcv++;
			LOG_MESSAGE( "Command %d, type %d, onoff %d, sbo %d, qu %d, utr %d\n", cmd_recv.endereco, cmd_recv.tipo, 
					cmd_recv.onoff, cmd_recv.sbo, cmd_recv.qu, cmd_recv.utr);

			//TODO:check if both connections enabled and send to the one with monitored state valid
			if(command_variable(srv_main.con, commands[i].id, cmd_recv.onoff)<0){
				send_cmd_response_to_ihm(ihm_socket_send, &ihm_sock_addr, commands[i].nponto, ihm_station, 0); //CMD ERROR
				commands[i].num_of_cmd_error++;
				LOG_MESSAGE("Error writing %d to %s\n", cmd_recv.onoff, commands[i].id);
			} else {
				send_cmd_response_to_ihm(ihm_socket_send, &ihm_sock_addr, commands[i].nponto, ihm_station, 1); //CMD OK
				commands[i].num_of_cmd_ok++;
				LOG_MESSAGE("Done writing %d to %s\n", cmd_recv.onoff, commands[i].id);
			}
		}else{
			char * cmd_debug = (char *)&cmd_recv;
			LOG_MESSAGE("ERROR - command received %d not found! \n", cmd_recv.endereco);  
			for (i=0; i < sizeof(t_msgcmd); i++){
				LOG_MESSAGE(" %02x", cmd_debug[i]);
			}
			LOG_MESSAGE("\n");
		}
		free(msg_rcv);
	}
}
/*********************************************************************************************************/
static void * check_bkp_thread(void * parameter)
{
	LOG_MESSAGE("Backup Thread Started\n");
	while(running){
		if(check_backup(2000) == 0){
			running = 0;	
			LOG_MESSAGE("ERROR - Detected backup from other server - exiting\n");
		}
		if(SendT(bkp_socket,(void *)&bkp_signature, sizeof(unsigned int), &bkp_sock_addr) < 0){
			running = 0;
			LOG_MESSAGE("Error sending message to backup server\n");
		}
	}
	return NULL;
}

/*********************************************************************************************************/
static void * check_stats_thread(void * parameter)
{
	LOG_MESSAGE("Stats Thread Started\n");
	int i;

	while(running){
		stats_data_msg * msg_rcv;
		msg_rcv = (stats_data_msg *) WaitT(stats_socket_receive, 2000);	
		if(msg_rcv != NULL) {
			if (msg_rcv->cmd== GET_GLOBAL_COUNTERS) {
				stats_global_counters msg_send;
				msg_send.analog_ids =  num_of_analog_ids;	
				msg_send.digital_ids = num_of_digital_ids;	
				msg_send.event_ids =  num_of_event_ids;	
				msg_send.commands = num_of_commands;	
				msg_send.datasets = num_of_datasets;
				msg_send.analog_datasets = num_of_analog_datasets;
				msg_send.digital_datasets = num_of_digital_datasets;
				msg_send.event_datasets = num_of_event_datasets;
				if(SendT(stats_socket_send,(void *)&msg_send, sizeof(stats_global_counters), &stats_sock_addr) < 0){
					LOG_MESSAGE("Error sending message to backup server\n");
				}
			} else if (msg_rcv->cmd == GET_HMI_COUNTERS) {
				stats_hmi_counters msg_send;
				msg_send.report_msgs = num_of_report_msgs;
				msg_send.digital_msgs = num_of_digital_msgs;
				msg_send.analog_msgs = num_of_analog_msgs;
				if(SendT(stats_socket_send,(void *)&msg_send, sizeof(stats_hmi_counters), &stats_sock_addr) < 0){
					LOG_MESSAGE("Error sending message to backup server\n");
				}
			} else if (msg_rcv->cmd == GET_NPONTO_COUNTERS) {
				stats_nponto_counters msg_send;
				int found = 0;
				memset(&msg_send, 0xFF , sizeof(stats_nponto_counters));
				for (i=0;((i<num_of_analog_ids) && !found); i++) {
					if(analog_cfg[i].nponto == msg_rcv->nponto){
						memcpy(&msg_send, &analog_cfg[i], sizeof(stats_nponto_counters));
						found = 1;
					}
				}
				for (i=0;((i<num_of_digital_ids) && !found); i++) {
					if(digital_cfg[i].nponto == msg_rcv->nponto){
						memcpy(&msg_send, &digital_cfg[i], sizeof(stats_nponto_counters));
						found = 1;
					}
				}
				for (i=0;((i<num_of_event_ids) && !found); i++) {
					if(events_cfg[i].nponto == msg_rcv->nponto){
						memcpy(&msg_send, &events_cfg[i], sizeof(stats_nponto_counters));
						found = 1;
					}
				}
				if(SendT(stats_socket_send,(void *)&msg_send, sizeof(stats_nponto_counters), &stats_sock_addr) < 0){
					LOG_MESSAGE("Error sending message to backup server\n");
				}
			} else if (msg_rcv->cmd == GET_NPONTO_STATE) {
				stats_nponto_state msg_send;
				int found = 0;
				memset(&msg_send, 0xFF , sizeof(stats_nponto_state));
				for (i=0;((i<num_of_analog_ids) && !found); i++) {
					if(analog_cfg[i].nponto == msg_rcv->nponto){
						memcpy(&msg_send, &srv_main.analog[i], sizeof(stats_nponto_state));
						found = 1;
					}
				}
				for (i=0;((i<num_of_digital_ids) && !found); i++) {
					if(digital_cfg[i].nponto == msg_rcv->nponto){
						memcpy(&msg_send, &srv_main.digital[i], sizeof(stats_nponto_state));
						found = 1;
					}
				}
				for (i=0;((i<num_of_event_ids) && !found); i++) {
					if(events_cfg[i].nponto == msg_rcv->nponto){
						memcpy(&msg_send, &srv_main.events[i], sizeof(stats_nponto_state));
						found = 1;
					}
				}
				if(SendT(stats_socket_send,(void *)&msg_send, sizeof(stats_nponto_counters), &stats_sock_addr) < 0){
					LOG_MESSAGE("Error sending message to backup server\n");
				}
			} else if (msg_rcv->cmd == GET_CMD_COUNTERS) {
				stats_cmd_counters msg_send;
				memset(&msg_send, 0xFF , sizeof(stats_nponto_counters));
				for (i=0;i<num_of_commands; i++) {
					if(commands[i].nponto == msg_rcv->nponto){
						memcpy(&msg_send, &commands[i], sizeof(stats_cmd_counters));
						break;
					}
				}
				if(SendT(stats_socket_send,(void *)&msg_send, sizeof(stats_cmd_counters), &stats_sock_addr) < 0){
					LOG_MESSAGE("Error sending message to backup server\n");
				}
			 } else {
				 LOG_MESSAGE ("ERROR - unknown stats message %d\n", msg_rcv->cmd);
			 } 
			free(msg_rcv);
		}	
	}
	return NULL;
}

/*********************************************************************************************************/

int start_bkp_configuration(void){
	if(strcmp(bkp_addr,"no")==0) {
		LOG_MESSAGE("no iccp client backup configured\n");
		bkp_present=0;
	}else{
		bkp_enabled=1;
		bkp_present=10;
		LOG_MESSAGE("bkp enabled\n");
		if(create_bkp_comm() < 0){
			LOG_MESSAGE("Error, cannot open communication to bkp server\n");
			return -1;
		}
		while (running && bkp_present){
			if(check_backup(((rand()%400) + 600)) < 0){//random buffer timeout
				bkp_present--;
			}else{
				bkp_present=10;
			}
		}

		if(!running){
			return -1;
		}	

		//Send Message to backup in order to mantain it disabled	
		if((SendT(bkp_socket,(void *)&bkp_signature, sizeof(unsigned int), &bkp_sock_addr)) < 0){
			LOG_MESSAGE("Error sending message to backup server on initialization\n");
			return -1;
		}
		if ((SendT(bkp_socket,(void *)&bkp_signature, sizeof(unsigned int), &bkp_sock_addr)) < 0){
			LOG_MESSAGE("Error sending message to backup server on initialization\n");
			return -1;
		}

		bkp_thread = Thread_create(check_bkp_thread, NULL, false);
		Thread_start(bkp_thread);

		LOG_MESSAGE("Backup client not alive\n");
	}
	return 0;
}
/*********************************************************************************************************/
int start_stats_configuration(void){
	if(strcmp(stats_addr,"no")==0) {
		LOG_MESSAGE("no iccp client stats configured\n");
	}else{
		LOG_MESSAGE("stats enabled\n");
		if(create_stats_comm() < 0){
			LOG_MESSAGE("Error, cannot open communication to stats server\n");
			return -1;
		}
		
		stats_thread = Thread_create(check_stats_thread, NULL, false);
		Thread_start(stats_thread);
	}
	return 0;
}

/*********************************************************************************************************/
static int start_iccp(st_server_data * srv_data){
	int i;
	MmsError mmsError;
// DELETE DATASETS WHICH WILL BE USED
	LOG_MESSAGE("deleting data sets (dds)!\n");
	for (i=0; i < num_of_datasets; i++) {
		fflush(stdout);
		MmsConnection_deleteNamedVariableList(srv_data->con,&mmsError, IDICCP, dataset_conf[i].id);
		LOG_MESSAGE("dds = %d %\n", (i*100)/num_of_datasets );
	}
	LOG_MESSAGE("dds = %d %\n", (i*100)/num_of_datasets );

	// CREATE TRASNFERSETS ON REMOTE SERVER	
	LOG_MESSAGE("creating transfer sets (cts)!\n");
	for (i=0; i < num_of_datasets; i++){
		fflush(stdout);
		MmsValue *transfer_set_dig  = get_next_transferset(srv_data->con,IDICCP);
		if( transfer_set_dig == NULL) {	
			LOG_MESSAGE("Could not create transfer set for digital data\n");
			return -1;
		} else {
			strncpy(dataset_conf[i].ts, MmsValue_toString(transfer_set_dig), TRANSFERSET_NAME_SIZE);
			MmsValue_delete(transfer_set_dig);
		}
		LOG_MESSAGE("cts = %d %\n", (i*100)/num_of_datasets );
	}	
	LOG_MESSAGE("cts = %d %\n", (i*100)/num_of_datasets );

	// CREATE DATASETS WITH CUSTOM VARIABLES
	LOG_MESSAGE("creating data sets (cds)!\n");
	for (i=0; i < num_of_datasets; i++){
		fflush(stdout);
		create_dataset(srv_data->con, dataset_conf[i].id,i);
		LOG_MESSAGE("cds = %d %\n", (i*100)/num_of_datasets );
	}
	LOG_MESSAGE("cds = %d %\n", (i*100)/num_of_datasets );

	// WRITE DATASETS TO TRANSFERSET
	LOG_MESSAGE("Write/Read data sets (wrds)\n");
	for (i=0; i < num_of_datasets; i++){
		fflush(stdout);
		
		if(dataset_conf[i].type == DATASET_ANALOG){ 
			write_dataset(srv_data->con, IDICCP, dataset_conf[i].id, dataset_conf[i].ts, analog_buf, integrity_time, REPORT_INTERVAL_TIMEOUT|REPORT_OBJECT_CHANGES|REPORT_BUFFERED);
			if(read_dataset(srv_data, dataset_conf[i].id, i) < 0)
				return -1;
		}
		else if(dataset_conf[i].type == DATASET_DIGITAL){
			write_dataset(srv_data->con, IDICCP, dataset_conf[i].id, dataset_conf[i].ts, digital_buf,integrity_time, REPORT_INTERVAL_TIMEOUT|REPORT_OBJECT_CHANGES);
			if(read_dataset(srv_data, dataset_conf[i].id, i) < 0)
				return -1;
		}
		else if(dataset_conf[i].type == DATASET_EVENTS){
			write_dataset(srv_data->con, IDICCP, dataset_conf[i].id, dataset_conf[i].ts, events_buf, integrity_time, REPORT_OBJECT_CHANGES); //FIXME
			if(read_dataset(srv_data, dataset_conf[i].id, i) < 0)
				return -1;
		}
		else{
			LOG_MESSAGE("unknown write dataset type\n");
			return -1;
		}
		Thread_sleep(1100);//sleep 1.1s for different report times (better handling)
		LOG_MESSAGE("wrds = %d %\n", (i*100)/num_of_datasets );
	}
	LOG_MESSAGE("wrds = %d %\n", (i*100)/num_of_datasets );

	return 0;
}
/*********************************************************************************************************/
static void * check_connections_thread(void * parameter)
{
	LOG_MESSAGE("Connections Thread Started\n");
	//if connection not enabled or lost, restart all
	while(running){
		if (srv_main.enabled){
			if (check_connection(srv_main.con,IDICCP, &srv_main.error) < 0){
				srv_main.enabled=0;
			}
		} else {
			if(connect_to_iccp_server(&srv_main.con, srv1,srv2,srv3,srv4) == 0){
				MmsConnection_setInformationReportHandler(srv_main.con, informationReportHandler, (void *) &srv_main);
				Thread_sleep(10000);
				if((check_connection(srv_main.con,IDICCP, &srv_main.error) < 0) || (start_iccp(&srv_main)<0)){
					LOG_MESSAGE("could not start configuration for connection principal\n");
					running = 0;
				} else{
					srv_main.enabled=1;
				}

			}
		}
		if (srv_bckp.enabled){
			if (check_connection(srv_bckp.con,IDICCP, &srv_bckp.error) < 0){
				srv_bckp.enabled=0;
			}
		} else {
			if(connect_to_iccp_server(&srv_bckp.con, srv5,srv6,srv7,srv8) == 0){
	 			MmsConnection_setInformationReportHandler(srv_bckp.con, informationReportHandler, (void *) &srv_bckp);
	 			Thread_sleep(10000);
				if((check_connection(srv_bckp.con,IDICCP, &srv_bckp.error) < 0) || (start_iccp(&srv_bckp)<0)){
					LOG_MESSAGE("could not start configuration for connection backup\n");
					running = 0;
				} else
					srv_bckp.enabled=1;
			}
		}
		Thread_sleep(2000);
	}
	return NULL;

}
/*********************************************************************************************************/
int main (int argc, char ** argv){
	unsigned int i = 0;
	gettimeofday(&start, NULL);
	signal(SIGINT, sigint_handler);
	srv_main.con = MmsConnection_create();
	srv_bckp.con = MmsConnection_create();
	analog_queue.mutex = Semaphore_create(1); //semaphore
	digital_queue.mutex = Semaphore_create(1); //semaphore
    digital_mutex = Semaphore_create(1); //semaphore
    localtime_mutex = Semaphore_create(1); //semaphore

	// OPEN LOG FILE
	if (open_log_file() != 0) {
		printf("Error opening log file\n");
		return -1;
	}

	// READ CONFIGURATION FILE
	if (read_configuration() != 0) {
		LOG_MESSAGE("Error reading configuration\n");
		return -1;
	} else {
		LOG_MESSAGE("Start configuration with:\n  %d analog, %d digital, %d events, %d commands\n", num_of_analog_ids, num_of_digital_ids, num_of_event_ids, num_of_commands);
		LOG_MESSAGE("  %d datasets:\n   - %d analog\n   - %d digital \n   - %d events \n", num_of_datasets, num_of_analog_datasets, num_of_digital_datasets, num_of_event_datasets);
	}

#ifdef DATA_LOG
	// OPEN DATA LOG FILES	
	if(open_data_logs()<0) {
		LOG_MESSAGE("Error, cannot open configuration data log files\n");
		cleanup_variables();
		return -1;
	}
#endif

	//CHECK IF BACKUP ICCP CLIENT IS CONFIGURED AND START IT
	if(start_bkp_configuration() < 0){
		cleanup_variables();
		return -1;
	}
			
	//INITIALIZE IHM CONNECTION 
	if(strcmp(ihm_addr,"no")==0) {
		LOG_MESSAGE("no ihm configured\n");
	}else{
		ihm_enabled=1;
		if(create_ihm_comm() < 0){
			LOG_MESSAGE("Error, cannot open communication to ihm server\n");
			cleanup_variables();
			return -1;
		}
	}

	//START Stats Thread
	if(start_stats_configuration() < 0){
		cleanup_variables();
		return -1;
	}
	
	//INITIALIZE ICCP CONNECTIONs TO SERVERs 
	if(connect_to_iccp_server(&srv_main.con, srv1,srv2,srv3,srv4) < 0){
		LOG_MESSAGE("Warning, cannot connect to iccp pool main\n");
	} else{
		srv_main.enabled = 1;
	}
	if(connect_to_iccp_server(&srv_bckp.con, srv5,srv6,srv7,srv8) < 0){
		LOG_MESSAGE("Warning, cannot connect to iccp pool backup\n");
	} else {
		srv_bckp.enabled = 1;
	}
	
	if(!srv_main.enabled && !srv_bckp.enabled){
		LOG_MESSAGE("Error,Could not connect to any iccp servers\n");
	}
	
	// HANDLE REPORTS
	MmsConnection_setInformationReportHandler(srv_main.con, informationReportHandler, (void *) &srv_main);
	MmsConnection_setInformationReportHandler(srv_bckp.con, informationReportHandler, (void *) &srv_bckp);

	//START ICCP CONFIGURATION
	if(srv_main.enabled){
	   if(start_iccp(&srv_main)<0){
			LOG_MESSAGE("could not start configuration for connection principal\n");
	   }
	}
	if(srv_bckp.enabled){
	   if(start_iccp(&srv_bckp)<0){
			LOG_MESSAGE("could not start configuration for connection backup\n");
	   }
	}
		
	connections_thread = Thread_create(check_connections_thread, NULL, false);
	Thread_start(connections_thread);
	
	LOG_MESSAGE("ICCP Process Successfully Started!\n");

	// LOOP TO MANTAIN CONNECTION ACTIVE AND CHECK COMMANDS	
	while(running) {

		if(!ihm_enabled)
			Thread_sleep(2000);
		else{
			if(ihm_socket_receive > 0)
				check_commands();
	
			if( ihm_socket_send > 0){
				// EMPTY ANALOG QUEUE	
				Semaphore_wait(analog_queue.mutex);	
				if(analog_queue.size && ((get_time_ms()-analog_queue.time) > 4000)){//timeout to send analog buffered messages if not empty
					if(send_analog_list_to_ihm(ihm_socket_send, &ihm_sock_addr,analog_queue.npontos, ihm_station, analog_queue.values, analog_queue.states, analog_queue.size) <0){
						LOG_MESSAGE( "Error sending analog buffer \n");
					}else{
						num_of_analog_msgs++;
						analog_queue.size=0;
					}
				}
				Semaphore_post(analog_queue.mutex);	

				// EMPTY DIGITAL QUEUE	
				Semaphore_wait(digital_queue.mutex);	
				if(digital_queue.size && ((get_time_ms()- digital_queue.time) > 3000)){//timeout to send digital buffered messages if not empty
					if(send_digital_list_to_ihm(ihm_socket_send, &ihm_sock_addr,digital_queue.npontos, ihm_station,  digital_queue.states, digital_queue.size) <0){
						LOG_MESSAGE( "Error sending digital buffer \n");
					}else{
						num_of_digital_msgs++;
						digital_queue.size=0;
					}
				}
				Semaphore_post(digital_queue.mutex);	
			}
		}

	}

	cleanup_variables();

	return 0;
}
