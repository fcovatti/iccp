#ifndef COMM_H_INCLUDED
#define COMM_H_INCLUDED


#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

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
} t_msgsup;

//Em "info" colocar os dados para valor ponto flutuante ou digital

// para o tipo 1 é somente 1 byte
typedef struct { // tipo 1
	unsigned char iq;  // informaçao com qualificador no formato iec104
} digital_seq;
/*
typedef struct { // tipo 30
	unsigned char iq;  // informaçao com qualificador no formato iec104
	unsigned short ms;   // milisegundos
	unsigned char min;
	unsigned char hora;
	unsigned char dia;
	unsigned char mes;
	unsigned char ano;
} digital_w_time7_seq;
*/

typedef struct { // tipo 30
	unsigned char iq;  // informaçao com qualificador no formato iec104
	unsigned short ms;   // milisegundos
	//unsigned char ms[2];   // milisegundos
	unsigned char min;   // milisegundos
	//unsigned char ms;
//	unsigned char min;//ano;
	unsigned char hora;//min;
	unsigned char dia;//hora
	unsigned char mes;//dia;
	unsigned char ano;//mes;
//	unsigned char min;
} __attribute__((packed))digital_w_time7_seq; 

typedef struct { // tipo 13
	float fr;              // valor em ponto flutuante
	unsigned char qds;     // qualificador do ponto no formato iec104
} flutuante_seq;

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
} t_msgcmd;

//#define DEBUG_MSGS
#define PORT_IHM_TRANSMIT  	8099
#define PORT_IHM_LISTEN   	8098

int prepare_Send(char * addr, int port, struct sockaddr_in * server_addr);

int SendT(int socketfd, void * msg, int msg_size, struct sockaddr_in * server_addr);

void send_digital_to_ihm(int socketfd, struct sockaddr_in * serv_sock_addr,unsigned int nponto,unsigned char utr_addr,unsigned char ihm_station, unsigned char state, time_t time_stamp, unsigned short time_stamp_extended, char report);

void send_analog_to_ihm(int socketfd, struct sockaddr_in * serv_sock_addr,unsigned int nponto,unsigned char utr_addr, unsigned char ihm_station, float value, unsigned char state, char report);
#endif
