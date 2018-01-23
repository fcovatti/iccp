#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include "comm.h"
#include <my_global.h>
#include <mysql.h>
#include <time.h>

static int hist_socket_receive=0;
static MYSQL *con;
static char query[100];
static char longquery[15000];
static unsigned int events_msgs;
static unsigned int digital_msgs;
static unsigned int analog_msgs;
static unsigned int error_msgs;
static unsigned int should_be_type_30;

static int running = 1; //used on handler for signal interruption
/*********************************************************************************************************/
static void sigint_handler(int signalId)
{
	running = 0;
}
/*********************************************************************************************************/
static void finish_with_error(MYSQL *con)
{
	fprintf(stderr, "%s\n", mysql_error(con));
	mysql_close(con);
	running = 0;
}
/*********************************************************************************************************/
static int create_hist_comm(){
	hist_socket_receive = prepare_Wait(PORT_IHM_TRANSMIT);
	if(hist_socket_receive < 0){
		printf("could not create UDP socket to listen to IHM\n");
		return -1;
	}
	printf("Created UDP local socket for IHM Port %d\n",PORT_IHM_TRANSMIT);
	return 0;
}
/*********************************************************************************************************/
static int create_db_comm(){

	con = mysql_init(NULL);

	if (con == NULL) 
	{
		fprintf(stderr, "%s\n", mysql_error(con));
		return -1;
	}  
	//if (mysql_real_connect(con, "localhost", "user12", "34klq*", 
	if (mysql_real_connect(con, "localhost", NULL, NULL, 
				"test", 0, NULL, 0) == NULL) 
	{
		finish_with_error(con);
		return -1;
	}    
	return 0;
}
/*********************************************************************************************************/
static int check_packet(){
	char * msg_rcv;
	unsigned int signature;
	t_msgsup *msg;
	t_msgsupsq *msg_sq;
	msg_rcv = WaitT(hist_socket_receive, 2000);	
	if(msg_rcv != NULL) {
		memcpy(&signature, msg_rcv, sizeof(unsigned int));
		if(signature==IHM_SINGLE_POINT_SIGN){
			msg=(t_msgsup *)msg_rcv;
			if(msg->tipo==30){
				digital_w_time7_seq *info;
				info=(digital_w_time7_seq *)msg->info;
				events_msgs++;
				//printf("%06d %02x %02d%02d%02d %02d%02d%02d%03d\n", msg->endereco, info->iq, info->dia, info->mes, 
				//		info->ano, info->hora, info->min, info->ms/1000, info->ms%1000 );
				memset(query,0,100);
				snprintf(query, 100, "INSERT INTO sde VALUES('%d','0','20%02d-%02d-%02d','%02d:%02d:%02d', '%d', '%d', '0', NULL)", 
						msg->endereco, info->ano, info->mes, info->dia, info->hora, info->min, info->ms/1000, info->ms%1000, info->iq);
				printf("query %s\n", query);
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
				//printf("wrong type %d\n",msg->tipo);
			}
		}
		else if(signature==IHM_POINT_LIST_SIGN){
			unsigned int i,nponto,total;
			msg_sq=(t_msgsupsq *)msg_rcv;
			if(msg_sq->tipo==1){
				digital_seq * info;
				time_t t = time(NULL);
				struct tm tm = *localtime(&t);
				nponto=0;
				memset(longquery,0,15000);
				snprintf(longquery, 65, "INSERT INTO h2018_01 (NPONTO, DATA, HORA, VALOR, FLAGS) VALUES ");
				for (i=0;i<msg_sq->numpoints;i++){
					memcpy(&nponto, &msg_sq->info[i*(sizeof(digital_seq)+sizeof(int))], sizeof(int));
					info=(digital_seq *)&msg_sq->info[i*(sizeof(digital_seq)+sizeof(int))+sizeof(int)];
					snprintf(longquery+strlen(longquery), 65, "('%d','%02d-%02d-%02d','%02d:%02d:00','0','%d'),", 
					nponto, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, info->iq);
				}
				longquery[strlen(longquery)-1]=';';
				if (mysql_query(con,longquery)) {
					finish_with_error(con);
					return -1;
				}
				//printf("query %s \n", longquery);
				digital_msgs++;	
			}else if(msg_sq->tipo==13){
				flutuante_seq *info;
				time_t t = time(NULL);
				struct tm tm = *localtime(&t);
				nponto=0;
				memset(longquery,0,15000);
				snprintf(longquery, 65, "INSERT INTO h2018_01 (NPONTO, DATA, HORA, VALOR, FLAGS) VALUES ");
				for (i=0;i<msg_sq->numpoints;i++){
					memcpy(&nponto, &msg_sq->info[i*(sizeof(flutuante_seq)+sizeof(int))], sizeof(int));
					info=(flutuante_seq *)&msg_sq->info[i*(sizeof(flutuante_seq)+sizeof(int))+sizeof(int)];
					snprintf(longquery+strlen(longquery), 65, "('%d','%02d-%02d-%02d','%02d:%02d:00','%.2f','%d'),", 
					nponto, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, info->fr, info->qds);
				}
				longquery[strlen(longquery)-1]=';';
				if (mysql_query(con,longquery)) {
					finish_with_error(con);
					return -1;
				}
				//printf("query %s \n", longquery);
				analog_msgs++;
			}else{
				error_msgs++;
				//printf("wrong gi type %d\n",msg_sq->tipo);
			}
		}
		else{
			error_msgs++;
			//printf("wrong signature %d\n", signature);
		}
	}
	return 0;
}
/*********************************************************************************************************/
int main (int argc, char ** argv){
	signal(SIGINT, sigint_handler);
	if(create_db_comm() <0){
		printf("could not create db comm\n");
		return -1;
	}

	if(create_hist_comm() <0){
		printf("could not create hist comm\n");
		finish_with_error(con);
		return -1;
	}
	
	while(running){
		check_packet();
	}
	printf("Total Receive %d - A:%d   D:%d   E:%d (%d) | Error: %d\n", (digital_msgs+analog_msgs+events_msgs),
			analog_msgs, digital_msgs, events_msgs, should_be_type_30, error_msgs);

	close(hist_socket_receive);
	mysql_close(con);
	return 0;
}
