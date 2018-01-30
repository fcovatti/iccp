#ifndef COMM_H_INCLUDED
#define COMM_H_INCLUDED


#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif


#define MAX_STR_NAME 35


//---------------------------------------------------------------------------
//Mandar mensagem UDP na porta 8099, uma para cada valor de ponto, no
//seguinte formato:
typedef struct {
	unsigned int signature;  // 0x53535353, valor fixo
	unsigned int endereco; // endereço do ponto (nponto), não limitado a 65535
	unsigned int tipo; // código do tipo IEC, pode ser somente o tipo 	digital simples com ou sem  tag e o 13 para analógicos
	unsigned int prim; // endereço da estação primária, pode colocar zero
	unsigned int sec; // originator address (pode colocar zero)
	unsigned int causa; // código causa iec (20=GI, 3=exceção) , ligar o bit	0x40 para confirmação OK de comando
	unsigned int taminfo; // tamanho dos dados em info
	unsigned char info[10]; // dado no formato padrão iec104
	//unsigned char info[255]; // dado no formato padrão iec104
} __attribute__((packed)) t_msgsup;

//---------------------------------------------------------------------------
typedef struct {
	unsigned int signature;  // 0x64646464, valor fixo
	unsigned int numpoints; // numero de pontos a serem enviados
	unsigned int tipo; // código do tipo IEC, pode ser somente o tipo 	digital simples com ou sem  tag e o 13 para analógicos
	unsigned int prim; // endereço da estação primária, pode colocar zero
	unsigned int sec; // originator address (pode colocar zero)
	unsigned int causa; // código causa iec (20=GI, 3=exceção) , ligar o bit	0x40 para confirmação OK de comando
	unsigned int taminfo; // tamanho dos dados em info
	unsigned char info[1400]; // dado no formato padrão iec104
} __attribute__((packed)) t_msgsupsq;

//Em "info" colocar os dados para valor ponto flutuante ou digital

// para o tipo 1 é somente 1 byte
typedef struct { // tipo 1
	unsigned char iq;  // informaçao com qualificador no formato iec104
} __attribute__((packed)) digital_seq;

typedef struct { // tipo 30
	unsigned char iq;  // informaçao com qualificador no formato iec104
	unsigned short ms;   // milisegundos
	unsigned char min;   // milisegundos
	unsigned char hora;//min;
	unsigned char dia;//hora
	unsigned char mes;//dia;
	unsigned char ano;//mes;
} __attribute__((packed))digital_w_time7_seq; 

typedef struct { // tipo 13
	float fr;              // valor em ponto flutuante
	unsigned char qds;     // qualificador do ponto no formato iec104
}  __attribute__((packed)) flutuante_seq;

//Para comando é mais fácil ainda, vai a mensagem no sentido inverso
//(escuta na 8098) no formato.

typedef struct
{
	unsigned int signature; // 0x4b4b4b4b
	unsigned int endereco;
	unsigned int tipo;
	unsigned int onoff;
	unsigned int sbo;
	unsigned int qu;
	unsigned int utr;
} __attribute__((packed)) t_msgcmd;

//#define DEBUG_MSGS
#define PORT_IHM_TRANSMIT  	8099
#define PORT_IHM_LISTEN   	8098

#define PORT_STATS_TRANSMIT   	8112
#define PORT_STATS_LISTEN   	8113

#define PORT_ICCP_BACKUP   	8102

#define	ICCP_BACKUP_SIGNATURE	0xA5A5A5A5
#define	IHM_SINGLE_POINT_SIGN	0x53535353
#define	IHM_POINT_LIST_SIGN		0x64646464
#define	ICCP_STATS_SIGNATURE	0xC9C9C9C9

#define MAX_MSGS_SQ_ANALOG	155
#define MAX_MSGS_SQ_DIGITAL	280
#define MAX_MSGS_GI_ANALOG	125
#define MAX_MSGS_GI_DIGITAL	250



int prepare_Wait(int port);

void * WaitT(unsigned int socketfd, int timeout_ms);

int prepare_Send(char * addr, int port, struct sockaddr_in * server_addr);

int SendT(int socketfd, void * msg, int msg_size, struct sockaddr_in * server_addr);

int prepareServerAddress(char* address, int port, struct sockaddr_in * server_addr); 

int send_digital_to_ihm(int socketfd, struct sockaddr_in * serv_sock_addr,unsigned int nponto,unsigned char ihm_station, unsigned char state, time_t time_stamp, unsigned short time_stamp_extended, char report);

int send_digital_list_to_ihm(int socketfd, struct sockaddr_in * server_sock_addr,unsigned int * npontos, unsigned char ihm_station, unsigned char * states, int list_size);

int send_analog_to_ihm(int socketfd, struct sockaddr_in * serv_sock_addr,unsigned int nponto, unsigned char ihm_station, float value, unsigned char state, char report);

int send_analog_list_to_ihm(int socketfd, struct sockaddr_in * server_sock_addr,unsigned int * npontos, unsigned char ihm_station, float * values, unsigned char * states, int list_size);

int send_cmd_response_to_ihm(int socketfd, struct sockaddr_in * server_sock_addr,unsigned int nponto, unsigned char ihm_station, char cmd_ok);

#endif
