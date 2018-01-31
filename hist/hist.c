#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include "comm.h"
#include <my_global.h>
#include <mysql.h>
#include <time.h>
/*****************READ ME******************
 *
 *  Table sde and val_tr must be created
 *  Reference Table h0000_00 must be created
 *  Support to nponto values until 1 million (database)
 *  Writing events on receive (ignoring duplicate entries)
 *  Writing digital data and analog once a day or on persistent change in a 5 minute window
 * */
#define QUERY_SIZE				100
#define LONG_QUERY_SIZE			15000
#define DATABASE_SIZE			1000000
#define FLAGS_INIT				255
#define LOG_MESSAGE(...)        do { print_time(log_file); fprintf(log_file, __VA_ARGS__); fflush(log_file); } while(false)

static FILE * log_file = NULL;
static int hist_socket_receive=0;
static MYSQL *con;
static char query[QUERY_SIZE];
static char longquery[LONG_QUERY_SIZE];
static unsigned int events_msgs;
static unsigned int digital_msgs;
static unsigned int analog_msgs;
static unsigned int error_msgs;
static unsigned int should_be_type_30;
typedef struct {
	float value;
	unsigned char flags;
	unsigned int lastchange;
} database;

static database data[DATABASE_SIZE];
static int currday=-1; //day control in order to write at least once every day to the table
static int currmon=-1; //day control in order to write at least once every day to the table
static int running = 1; //used on handler for signal interruption
/*********************************************************************************************************/
static void sigint_handler(int signalId)
{
	running = 0;
}
/*********************************************************************************************************/
static void print_time(FILE * log_file){

	time_t t = time(NULL);
	struct tm now = {0};

	localtime_r(&t, &now );
	fprintf(log_file, "%04d/%02d/%02d %02d:%02d:%02d - ", now.tm_year+1900, now.tm_mon+1, now.tm_mday,now.tm_hour, now.tm_min,now.tm_sec);
}

/*********************************************************************************************************/
static void finish_with_error(MYSQL *con)
{
	LOG_MESSAGE("%s\n", mysql_error(con));
	mysql_close(con);
	running = 0;
}
/*********************************************************************************************************/
static int create_hist_comm(){
	hist_socket_receive = prepare_Wait(PORT_IHM_TRANSMIT);
	if(hist_socket_receive < 0){
		LOG_MESSAGE("could not create UDP socket to listen to IHM\n");
		return -1;
	}
	LOG_MESSAGE("Created UDP local socket for IHM Port %d\n",PORT_IHM_TRANSMIT);
	return 0;
}
/*********************************************************************************************************/
static int create_db_comm(){

	con = mysql_init(NULL);

	if (con == NULL) 
	{
		LOG_MESSAGE("%s\n", mysql_error(con));
		return -1;
	}  
	if (mysql_real_connect(con, "localhost", NULL, NULL, 
				"bancotr", 0, NULL, 0) == NULL) 
	{
		finish_with_error(con);
		return -1;
	}    
	LOG_MESSAGE("Created DB connection\n");
	return 0;
}
/**************l*******************************************************************************************/
static int check_packet(){
	char * msg_rcv;
	unsigned int signature;
	unsigned char state;
	struct tm tm;
	time_t t;
	t_msgsup *msg;
	t_msgsupsq *msg_sq;
	
	msg_rcv = WaitT(hist_socket_receive, 2000);	
	if(msg_rcv != NULL) {
		
		t = time(NULL);
		tm = *localtime(&t);//get local time

		//first of all, if month has changed, or first time check/create a new table
		if(tm.tm_mon!=currmon){
			MYSQL_RES * result;
			int found=0;
			memset(query,0,QUERY_SIZE);
			snprintf(query, QUERY_SIZE, "show tables like 'h%d_%02d'",tm.tm_year+1900, tm.tm_mon+1);
			if (mysql_query(con,query)) {
				finish_with_error(con);
				return -1;
			}
			result=mysql_store_result(con);
			if(result){
				if(mysql_num_rows(result)){
					found=1;
				}
			}	
			mysql_free_result(result);
			if(!found){
				memset(query,0,QUERY_SIZE);
				snprintf(query, QUERY_SIZE, "create table `h%d_%02d` like `h0000_00`",tm.tm_year+1900, tm.tm_mon+1);
				LOG_MESSAGE("%s\n",query);
				if (mysql_query(con,query)) {
					finish_with_error(con);
					return -1;
				}
			}
			currmon=tm.tm_mon;
		}

		//copy type of message
		memcpy(&signature, msg_rcv, sizeof(unsigned int));

		//digital report 
		if(signature==IHM_SINGLE_POINT_SIGN){
			msg=(t_msgsup *)msg_rcv;
			if(msg->tipo==30){
				digital_w_time7_seq *info;
				info=(digital_w_time7_seq *)msg->info;
				events_msgs++;

				state=(info->iq&0xF7);

				//first insert into history if it has already been writen once (events are never writen)
				if(msg->endereco<DATABASE_SIZE && data[msg->endereco].flags != FLAGS_INIT){
					memset(query,0,QUERY_SIZE);
					snprintf(query, QUERY_SIZE, "INSERT IGNORE INTO h%d_%02d VALUES ('%d','%02d-%02d-%02d','%02d:%02d:%02d','%d','%d')", 
						tm.tm_year+1900, tm.tm_mon+1, msg->endereco, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,(state&0x01), (state&0xFD));
				//	LOG_MESSAGE("%s\n", query);
					if (mysql_query(con,query)) {
						finish_with_error(con);
						return -1;
					}
				}

				//always update val_tr
				memset(query,0,QUERY_SIZE);
				snprintf(query, QUERY_SIZE, "REPLACE INTO val_tr VALUES ('%d','%02d-%02d-%02d','%02d:%02d:%02d','%d','%d')", 
					 msg->endereco, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,(state&0x01),state);
				
				if (mysql_query(con,query)) {
					finish_with_error(con);
					return -1;
				}

				//than insert into sde
				memset(query,0,QUERY_SIZE);
				snprintf(query, QUERY_SIZE, "INSERT IGNORE INTO sde VALUES('%d','0','20%02d-%02d-%02d','%02d:%02d:%02d', '%d', '%d', '0', NULL)", 
						msg->endereco, info->ano, info->mes, info->dia, info->hora, info->min, info->ms/1000, info->ms%1000, state);
				//LOG_MESSAGE("%s\n", query);
				if (mysql_query(con,query)) {
					finish_with_error(con);
					return -1;
				}
			}	
			else if(msg->tipo==1){
				events_msgs++;	
				should_be_type_30++;
			}
			else{
				error_msgs++;
			}
		} //if it is a list of data
		else if(signature==IHM_POINT_LIST_SIGN){
			unsigned int i,nponto,total,insertone=0;
			msg_sq=(t_msgsupsq *)msg_rcv;
			nponto=0;

			//new day or starting process, reset flags in order to write again all data (once a day)
			if(tm.tm_mday!=currday){
				for(i=0;i<DATABASE_SIZE;i++)
					data[i].flags=FLAGS_INIT;
				currday=tm.tm_mday;
			}

			//if digital
			if(msg_sq->tipo==1){
				digital_seq * info;

				//first update val_tr table
				memset(longquery,0,LONG_QUERY_SIZE);
				snprintf(longquery, QUERY_SIZE, "REPLACE INTO val_tr (NPONTO, DATA, HORA, VALOR, FLAGS) VALUES ");
				for (i=0;i<msg_sq->numpoints;i++){
					memcpy(&nponto, &msg_sq->info[i*(sizeof(digital_seq)+sizeof(int))], sizeof(int));
					info=(digital_seq *)&msg_sq->info[i*(sizeof(digital_seq)+sizeof(int))+sizeof(int)];

					state=(info->iq&0xF7);

					snprintf(longquery+strlen(longquery), QUERY_SIZE, "('%d','%02d-%02d-%02d','%02d:%02d:%02d','%d','%d'),", 
					nponto, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min,tm.tm_sec, (state&0x01), (state&0xFD));
				}
				longquery[strlen(longquery)-1]=';';
				if (mysql_query(con,longquery)) {
					finish_with_error(con);
					return -1;
				}

				//than update history table
				memset(longquery,0,LONG_QUERY_SIZE);
				snprintf(longquery, QUERY_SIZE, "INSERT IGNORE INTO h%d_%02d (NPONTO, DATA, HORA, VALOR, FLAGS) VALUES ",tm.tm_year+1900, tm.tm_mon+1);
				for (i=0;i<msg_sq->numpoints;i++){
					memcpy(&nponto, &msg_sq->info[i*(sizeof(digital_seq)+sizeof(int))], sizeof(int));
					info=(digital_seq *)&msg_sq->info[i*(sizeof(digital_seq)+sizeof(int))+sizeof(int)];
					if(data[nponto].flags!=info->iq){
						state=(info->iq&0xF7);
						snprintf(longquery+strlen(longquery), QUERY_SIZE, "('%d','%02d-%02d-%02d','%02d:%02d:%02d','%d','%d'),", 
							nponto, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min,tm.tm_sec, (state&0x01), (state&0xFD));
						insertone=1;
						data[nponto].flags=info->iq;
					}
				}
				if(insertone){
					longquery[strlen(longquery)-1]=';';
					if (mysql_query(con,longquery)) {
						finish_with_error(con);
						return -1;
					}
				}
				digital_msgs++;	
			
			//if analog data
			}else if(msg_sq->tipo==13){
				flutuante_seq *info;
			
				//first update val_tr
				memset(longquery,0,LONG_QUERY_SIZE);
				snprintf(longquery, QUERY_SIZE, "REPLACE INTO val_tr (NPONTO, DATA, HORA, VALOR, FLAGS) VALUES ");
				for (i=0;i<msg_sq->numpoints;i++){
					
					memcpy(&nponto, &msg_sq->info[i*(sizeof(flutuante_seq)+sizeof(int))], sizeof(int));
					info=(flutuante_seq *)&msg_sq->info[i*(sizeof(flutuante_seq)+sizeof(int))+sizeof(int)];
	
					snprintf(longquery+strlen(longquery), QUERY_SIZE, "('%d','%02d-%02d-%02d','%02d:%02d:%02d','%.2f','%d'),", 
						nponto, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,info->fr,info->qds);
				}
				longquery[strlen(longquery)-1]=';';
				if (mysql_query(con,longquery)) {
					finish_with_error(con);
					return -1;
				}

				//than update history
				memset(longquery,0,LONG_QUERY_SIZE);
				snprintf(longquery, QUERY_SIZE, "INSERT IGNORE INTO h%d_%02d (NPONTO, DATA, HORA, VALOR, FLAGS) VALUES ",tm.tm_year+1900, tm.tm_mon+1);
				for (i=0;i<msg_sq->numpoints;i++){
					memcpy(&nponto, &msg_sq->info[i*(sizeof(flutuante_seq)+sizeof(int))], sizeof(int));
					info=(flutuante_seq *)&msg_sq->info[i*(sizeof(flutuante_seq)+sizeof(int))+sizeof(int)];
					//write analog if flag has changed otherwise write if values have changed more than 0.5% or 0.5 within a minute
					//every five minutes write if value have changed more than 0.2% or at least 0.2
					if(data[nponto].flags!=info->qds || 
							((t-data[nponto].lastchange)>=60 && ((fabs(info->fr)>100 && (fabs(info->fr-data[nponto].value)/fabs(info->fr) > 0.005)) || 
							(fabs(info->fr)<=100 && fabs(info->fr-data[nponto].value)>=0.5) )) ||
						   	((t-data[nponto].lastchange)>=300 && ((fabs(info->fr)>100 && (fabs(info->fr-data[nponto].value)/fabs(info->fr) > 0.002)) || 
							(fabs(info->fr)<=100 && fabs(info->fr-data[nponto].value)>=0.2) ))){ 
						snprintf(longquery+strlen(longquery), QUERY_SIZE, "('%d','%02d-%02d-%02d','%02d:%02d:%02d','%.2f','%d'),", 
						nponto, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, info->fr, info->qds);
						insertone=1;
						data[nponto].flags =info->qds;
						data[nponto].value =info->fr;
						data[nponto].lastchange = t;
					}
				}
				if(insertone){
					longquery[strlen(longquery)-1]=';';
					if (mysql_query(con,longquery)) {
						finish_with_error(con);
						return -1;
					}
				}
				analog_msgs++;
			}else{
				error_msgs++;
			}
		}
		else{
			error_msgs++;
		}
		free(msg_rcv);
	}
	return 0;
}
/*********************************************************************************************************/
static int open_log_file(){
	/*****************
	 *          * OPEN LOG FILES
	 *            * **********/
	time_t t = time(NULL);
	struct tm now = *localtime(&t);
	char flog[MAX_STR_NAME];

	snprintf(flog,MAX_STR_NAME, "/tmp/hist_info-%04d%02d%02d%02d%02d.log", now.tm_year+1900, now.tm_mon+1, now.tm_mday,now.tm_hour, now.tm_min);
	
	log_file = fopen(flog, "w");
	if(log_file==NULL){
		printf("Error, cannot open log file %s\n",flog);
		fclose(log_file);
		return -1;
	}
	return 0;
}

/*********************************************************************************************************/
int main (int argc, char ** argv){
	int i;
	signal(SIGINT, sigint_handler);
	
	if (open_log_file() != 0) {
		printf("Error opening log file\n");
		return -1;
	}

	if(create_db_comm() <0){
		LOG_MESSAGE("could not create db comm\n");
		return -1;
	}

	if(create_hist_comm() <0){
		LOG_MESSAGE("could not create hist comm\n");
		finish_with_error(con);
		return -1;
	}
	
	while(running){
		check_packet();
	}
	LOG_MESSAGE("Total Receive %d - A:%d   D:%d   E:%d (%d) | Error: %d\n", (digital_msgs+analog_msgs+events_msgs),
			analog_msgs, digital_msgs, events_msgs, should_be_type_30, error_msgs);

	close(hist_socket_receive);
	mysql_close(con);
	return 0;
}
