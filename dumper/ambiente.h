/*
                                                                            
                    A M B I E N T E   D E   S I S T E M A   O P E R A C I O N A L   E   R E D E                                                     

                                      DEFINI��ES DE CONSTANTES E FUN��ES

                                               04-05-2009
                                                                            
                                                                            
*/

#ifndef _AMBIENTE_H_INCLUDED
#define _AMBIENTE_H_INCLUDED
//#pragma check_stack(off)				// dispensa teste de pilha
//#pragma pack(1)                        	// campos das estruturas contiguos   
										// constantes logicas
#define TRUE  			-1              //  verdadeiro       
#define FALSE 			0               //  falso            
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stddef.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <errno.h>
#include <linux/tcp.h>
#include <fcntl.h>
#include <sys/reboot.h>

#define DEBUG_MSGS

//           							DEFINICAO DE TIPOS DO SISTEMA OPERACIONAL

//#define  process 		void            // tarefa rodando sob o sistema operacional
//#define  timeout 		interrupt    // rotina temporizada (relogio)
#define  timeout 		void    // rotina temporizada (relogio)
typedef  unsigned char 	relogio;        // identificador de relogio devolvido pelo sistema operacional
static char texto_tela[30][200];
#define semaforo sem_t
typedef  unsigned long 	dword;          // grandeza de 4 byte sem sinal
typedef  unsigned 		word;           // grandeza de 2 bytes sem sinal
typedef  unsigned char 	byte;           // grandeza de 1 byte sem sinal
typedef  unsigned char 	bool;           // grandeza de 1 byte p/valores l�gicos
typedef  char * 		string;         // vetor de caracteres terminado por 0 (binario zero)
typedef struct                              		
   {                                    // buffer do sistema operacional
   int        LENGH;                    //  numero total de bytes de dados
   char       DATA[1];                  //  sequencia de dados (tamanho variavel)
   }                    buffer;                               		


//                        				PRIMITIVAS DO SISTEMA OPERACIONAL

//extern pid Instala(process *,string,unsigned,unsigned);
/*     Coloca uma rotina a executar como um processo concorrente, com prioridade. 
       Devolve o identificador do processo. Se igual a 0, houve erro.
       #1 : ponteiro para o processo;
       #2 : identificacao mnemonica do processo (4 caracteres);
       #3 : tamanho da pilha do processo.
       #4 : prioridade (0 a 9, quanto maior o numero maior a prioridade).   
*/

static inline void Termina(void){
	//TODO end a process
}
/*     Exclui o processo ativo da estrutura de execucao concorrente.        
*/

static inline void Residente(void) {
	while(TRUE)
		sleep(10);
}
/*     Mantem processos ativados pelo programa residentes, ocupando a area de
       memoria efetivamente usada ate' o momento. Volta ao sistema.         
*/

extern void Foregrnd(void);
/*     Espera o termino do ultimo dos processos ativados e volta ao sistema.
*/

#define ProcAtiv getpid
/*     Informa o identificador do processo ativo sob o XDOS.                
*/

extern void TrocProc(void);
/*     Libera o processador para outro processo rodar.                      
*/

//extern pid Identif(string);
/*     Devolve o identificador do processo a partir do mnemonico do mesmo.
       #1 : string com o identificador mnemonico do processo.               
*/

static int inline Mnemonico(int pid ,string str){
	//TODO: implement for iec101
	return 0;
}
/*     Devolve o mnemonico do processo a partir do identificador do mesmo.
       #1 : identificador do processo (PID);
       #2 : string com o identificador mnemonico do processo.
       RETORNO:
       = 0: sucesso, string preenchido;
       <>0: pid invalido.                                                   
*/

/**************************************************************************************************/
static inline void P(sem_t * sem_key )
{
	sem_post(sem_key);
}

/**************************************************************************************************/
static inline void V( sem_t * sem_key )
{
	sem_wait(sem_key);
}
// extern void P(semaforo *);
/*     Realiza uma operacao P sobre uma variavel do tipo semaforo.
       #1 : endereco do semaforo.                                           
*/

// extern void V(semaforo *);
/*     Realiza uma operacao V sobre uma variavel do tipo semaforo.
       #1 : endereco do semaforo.                                           
*/

static inline void InicS(sem_t *sem_key,int initial){
	sem_init(sem_key, 0 , initial);
}
/*     Inicializa uma variavel do tipo semaforo.
       #1 : endereco do semaforo;
       #2 : valor inicial.                                                  
*/

static inline int _dos_open(char * file, char actions, FILE * fp) {

	printf("fopen file %s, action %d\n", file, actions);
	switch (actions) {
		case O_RDONLY:
			fp = fopen(file, "r");
			break;
		case O_RDWR:
			fp = fopen(file, "r+");
			break;
		case O_WRONLY:
			fp = fopen(file, "w");
			break;
		default:
			fp = NULL;
			break;
	}

	if(fp == NULL) {
		printf("fp null\n");
		return 1;
	}

	return 0;

}

static inline int _dos_creat(char * file, char actions, FILE * fp) {
	return _dos_open (file, actions, fp);
}

static inline int _dos_write(FILE * fp, void * msg, int msg_size, int * ret) {
	if (fwrite(msg, msg_size, 1, fp) != 1) {
			*ret = 1;
			return 1;
		}
		*ret = 0;
		return 0;
}

static inline int _dos_read(FILE * fp, void * msg, int msg_size, int * ret) {
	if (fread(msg, msg_size, 1, fp) != 1) {
		*ret = 1;
		return 1;
	}
	*ret = 0;
	return 0;
}

static inline void _dos_close(FILE * fp) {
	fclose(fp);
}

//extern char Send(pid,void  *);
/*     Envia uma mensagem para outro processo. 
       Devolve 0 se houve sucesso.
       #1 : identificador do processo destino;
       #2 : ponteiro para o buffer de mensagem.                             
*/

//extern char Signal(pid);
/*     Envia uma sinaliza��o para que outro processo fique pronto. 
       Devolve 0 se houve sucesso.
       #1 : identificador do processo destino;
*/

static inline unsigned long Address(void) {

	char hostname[128] = "eth0";
	//int i;
	struct hostent *he;
	struct in_addr **addr_list;
	//struct in_addr addr;

	if( gethostname(hostname, sizeof hostname) < 0)
		printf("error gethostname %s \n", hostname);

	he = gethostbyname(hostname);

	if (he == NULL) { // do some error checking
		herror("gethostbyname"); // herror(), NOT perror()
		return 0;
	}

	//print information about this host:
	addr_list = (struct in_addr **)he->h_addr_list;

	/*for(i = 0; addr_list[i] != NULL; i++) {
			printf("get address: %s ", inet_ntoa(*addr_list[i]));
	}*/
	return addr_list[0]->s_addr;
}
/*
         Informa o endere�o de rede desta esta��o.
         RETORNO:
            endere�o de rede desta esta��o.
*/
static inline char SendT(unsigned int port, void * msg, int msg_size) {
	struct sockaddr_in servAddr;
	int socketfd;

#ifdef TCPCLIENT_GETHOSTBYNAME
	struct hostent *server;
#endif

	/*1) Create a UDP socket*/
	if ( (socketfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
		fprintf(stderr, "Error creating udp socket. Aborting!\r\n");
		return -1;
	}
	memset(&servAddr, 0, sizeof(servAddr));

	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.s_addr = INADDR_ANY;

	/*2) Establish connection*/
#ifdef DEBUG_MSGS
	printf("Sending message size %d\n", msg_size);
#endif
	if ( sendto(socketfd, msg, msg_size, 0, (const struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
			fprintf(stderr, "Error connecting!\r\n");
			return -1;
		}

	close(socketfd);	
	
	return 0;
}
static int inline prepare_Wait(unsigned long addr, int port)
{
	int socketfd;
	   struct sockaddr_in servSock;
    	/*Create a UDP socket for incoming connections*/
		if ( (socketfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP) ) < 0 ) {
			printf("Error creating a udp socket port %d!\n", port);
			return -1;
		}

		memset(&servSock, 0, sizeof(servSock));

		servSock.sin_family = AF_INET;
		servSock.sin_port = htons(port);
		servSock.sin_addr.s_addr = htonl(addr);

		/* 2) Assign a port to a socket(bind)*/
		if ( bind(socketfd, (const struct sockaddr *)&servSock, sizeof(servSock) ) < 0) {
			close(socketfd);
			fprintf(stderr, "Error on bind port %d.. %s\r\n", port, strerror(errno));
			return -1;
		}
		printf("UDP socket initialization for port %d , address %lx, fd %d\n", port, addr, socketfd);

		return socketfd;

}
static inline void * WaitT(unsigned int socketfd, int timeout_ms) {
	void * buf = NULL;
	int n, ret;
	//fd_set masterfds;
	fd_set readfds;
	struct timeval timeout_sock;
	
	FD_ZERO(&readfds);
	FD_SET(socketfd, &readfds);
	//FD_ZERO(&masterfds);
	//FD_SET(0, &masterfds);
	//FD_ZERO(&readfds);
	//FD_SET(socketfd,&masterfds);
	//memcpy(&readfds, &masterfds, sizeof(fd_set));
	timeout_sock.tv_sec = timeout_ms/1000;
	timeout_sock.tv_usec = (timeout_ms%1000)*1000;
	//printf("waiting message socket %d\n", socketfd);
	if(timeout_ms)
		ret = select(socketfd + 1, &readfds, NULL, NULL, &timeout_sock);
	else
		ret = select(socketfd + 1, &readfds, NULL, NULL, NULL);
	//printf("waitT select ret %d\n", ret);
	if (ret < 0) {
		printf("select error socket %d\n", socketfd);
		//exit(1);
		return NULL;
	} else if (ret == 0) {
		//printf("select socket %d timeout\n", socketfd);
		return NULL;
	}
	if (FD_ISSET(socketfd, &readfds)) {

		buf = malloc(2000);
		n = recv(socketfd, buf, 2000, 0);
#ifdef DEBUG_MSGS
		printf("read %d bytes socketd %d\n", n, socketfd);
#endif
		if (n < 0) {
			printf("ERROR reading from socket %d\n", socketfd);
			free(buf);
			return NULL;
		}
	}
	return buf;
}
/*     Espera durante um tempo por uma mensagem de qualquer processo. 
       Devolve o ponteiro  para a mensagem ou NULL se estourou o tempo.
       #1 : tempo de espera em unidades de 55mseg. Se =0 espera ate' chegar.
*/

//extern char Wake(pid,void  *);
/*     Coloca uma mensagem no inicio da fila de outro processo, acordando-o se estiver esperando mensagem do processo ativo.
       Devolve 0 se houve sucesso, e diferente de 0 se houve erro.
       #1 : identificador do processo destino;
       #2 : ponteiro para o buffer de mensagem.                             
*/

#define AlocBuf malloc
/*     Aloca um buffer do gerenciador proprio. 
       Devolve o endereco do buffer ou NULL se nao houver memoria suficiente.
       #1 : tamanho do buffer em bytes;                                     
*/

#define LibBuf free
//extern char LibBuf(void  *);
/*     Devolve um buffer para o gerenciador de memoria do XDOS. 
       Retorna 0 se houve sucesso, senao o parametro esta' errado.
       #1 : ponteiro para um buffer anteriormente recebido por um WaitT.    
*/

static inline relogio CriaRel(void  * ptr_func,int param) 
{
	//TODO
	return 0;
}
/*     Conecta uma funcao na estrutura de execucao temporizada. 
       Devolve o numero do relogio alocado. Se = 0FFH, nao existe relogio disponivel.
       #1 : ponteiro para uma funcao definida como "";
       #2 : parametro a ser passado para a funcao na execucao.              
*/

static inline void ParteRel(relogio rel_num,unsigned start_time)
{
	//TODO
}
/*     Parte a contagem do tempo para execucao da funcao.
       #1 : numero do relogio anteriormente criado.
       #2 : tempo para disparo da funcao, em unidades de 55mseg.            
*/

static inline void ParaRel(relogio rel_num){
	//TODO
}
/*     Para definitivamente (ate' proximo ParteRelog) a contagem do tempo.
       #1 : numero do relogio anteriormente criado.                         
*/

extern void CongRel(relogio);
/*     Para temporariamente (ate' proximo DescRelog) a contagem do tempo.
       #1 : numero do relogio anteriormente criado.                         
*/

extern void DescRel(relogio);
/*     Reativa a contagem do tempo do relogio do ponto que havia parado.
       #1 : numero do relogio anteriormente criado.                         
*/

extern void LibeRel(relogio);
/*     Exclui a funcao da temporizacao, devolvendo o relogio ao sistema.
       #1 : numero do relogio anteriormente criado.                         
*/

//                      				FUNCOES AUXILIARES                                   

static inline void TextoDir(int line, int col, char * text, int attr)
{
	//TODO: print text
	char * str;
	int i;
	int ret;
	int size;

	//printf("TextoDir %d:%d %s\n", line, col, text);
	size = sizeof(text);
	if((200-col) < size)
		size = 200-col;

	//printf("TEXTO, line, col %d:%d %s\n",line, col, text);
	if(line > 30){
		printf("error fim tela! line %d, text %s\n", line, text);
		return;
	}
	if (col > 200) {
		printf("error fim tela! col %d, text %s\n", line, text);
		return;
	}

	ret = snprintf(&texto_tela[line][col],200-col, "%s", text);
	//ret = sprintf(&texto_tela[line][col],"%s", text);
	if(ret < 0)
		printf("error snprintf\n");

	//printf("%s\n", text);
	//printf("%d:%d: %s \n", line, col, text);
	for (i=0; i < 30; i++){
		str = texto_tela[i];
		printf("%02d|%s \n",i, str);
	}
	//printf("%s \n", text);
	return;
}
/*     Mostra um "string" na tela na posicao e com atributo especificado pelo usuario, usando acesso direto a memoria de video.
       #1 : numero da linha do video que deve comecar o texto.
       #2 : numero da coluna do inicio do texto.
       #3 : endereco do texto.
       #4 : atributo (cor de frente, cor de fundo, intensidade, piscante)   
*/

static inline void CharDir(int line_number, int col_number, char caracter, int attribute) {
	if(line_number > 30){
		printf("error fim tela! line %d, text %c\n", line_number, caracter);
		return;
	}
	if (col_number > 200) {
		printf("error fim tela! col %d, text %c\n", line_number, caracter);
		return;
	}
	texto_tela[line_number][col_number] = caracter;
	//TODO: Show chars on screen
}
/*     Mostra um caracter na tela na posicao e com atributo especificado pelo usuario, usando acesso direto a memoria de video.
       #1 : numero da linha do video que deve comecar o texto.
       #2 : numero da coluna do inicio do texto.
       #3 : caracter.
       #4 : atributo (cor de frente, cor de fundo, intensidade, piscante)   
*/

extern void Atributo(int, int, int, int);
/*     Muda o atributo de uma area da memoria de video.
       #1 : numero da linha do inicio da area de video.
       #2 : numero da coluna do inicio da area de video.
       #3 : numero de caracteres da area.
       #4 : atributo (cor de frente, cor de fundo, intensidade, piscante)   
*/

static inline void Cursor(int line, int colum) {
	//TODO implement cursor
	//printf("line %d, colum %d\n", line, colum);
}
/*     Coloca o cursor numa determinada posicao do video.
       #1 : numero da linha do video;
       #2 : numero da coluna do video.                                      
*/

static inline void LimpaTela(int line_init, int line_end){
	int i;
	for(i = line_init; i < line_end; i ++){
		memset(texto_tela[line_init], 0, 200);
	}
	//printf("cleaning lines from %d to %d\n", line_init, line_end);
}
/*     Limpa uma area da tela de video e posiciona o cursor no incio dela.
       #1 : numero da linha inicial da area (coluna = 0);
       #2 : numero da linha final da area, inclusive (coluna = 79);         
*/

static inline void Reboot(void){
	printf("rebooting\n");
	reboot(RB_AUTOBOOT);
}
/*     Reinicializa a maquina.                                              
*/

static inline void MoveBuf(void * src, void * dest, unsigned size){
	memcpy(dest,src,size);
}
/*     Move o conteudo de uma area de memoria para outra.
       #1 : ponteiro para o buffer origem.
       #2 : ponteiro para o buffer destino.
       #3 : numero de bytes do buffer.                                      
*/
struct st_queue {
	void * ptr;
	struct st_queue *next;
};

static inline int FilaIn(void * queue, void * ptr){
	if(queue == NULL) {
		queue = malloc(sizeof(struct st_queue));
		((struct st_queue *)queue)->ptr = ptr;
		((struct st_queue *)queue)->next = NULL;
	} else{
		((struct st_queue *)queue)->next = malloc(sizeof(struct st_queue));
		((struct st_queue *)queue)->next->next = NULL;
		((struct st_queue *)queue)->next->ptr = ptr;
	}
	return 1;
}
/*     Enfileira um buffer do XDOS a outros previamente enfileirados.
       #1 : ponteiro p/endereco do buffer inicio da fila.
            Se o conteudo e' NULL, a fila sera' criada;
       #2 : endereco do buffer a ser incluido na fila;
       RETORNO:
       = 0 : erro;
       = 1 : buffer enfileirado;                                            
*/

static inline void * FilaOut(void * queue){
	void * ptr;
	void * tmp;
	if(queue == NULL)
		return NULL;
	else {
		ptr = ((struct st_queue *)queue)->ptr;
		if (((struct st_queue *)queue)->next == NULL){
			free(queue);
		} else {
			tmp = queue;
			queue = ((struct st_queue *)queue)->next;
			free(tmp);
			}

		if(ptr == NULL)
			printf("ERROR - ptr was also freed\n");
		return ptr;

	}
}
/*     Retira um buffer do XDOS previamente enfileirado.
       #1 : ponteiro p/endereco do buffer inicio da fila.
       RETORNO:
       = NULL : fila vazia. Senao endereco do buffer retirado da fila.      
*/



//                                               MODULO NET.c
static inline char HostMain(void)
{
	//TODO: implement HostMain
	//TODO: tell other pc to switch via udp packet and check if this is the main one
	return TRUE;
}
/*
         Informa se este computador est� desempenhando a fun��o de principal em rela��o ao seu similar.
         RETORNO:
            TRUE: � o principal.
            FALSE: � o reserva;
*/

static inline char HostSwitch(void)
{
	printf("TODO: implement HostSwitch\n");
	return 0;
	//TODO: tell other pc to switch via udp packet
	/*{
	   udp_pkt PERG;

	   if ((!PID_IP) &&
	       (IdentificaIP() > 0))
	      return(1);
	   PERG.HD_CTR.LENGH = sizeof(ctr_header) - 2;
	   PERG.HD_CTR.SERV = 'M';
	   PERG.HD_CTR.PID = ATIVO;
	   PERG.HD_CTR.PTR = NULL;
	   PERG.HD_CTR.PROT = UDP;
	   if (Send(PID_IP,&PERG))
	      return(2);
	   return(0);
	}*/
}
/*
         Altera o modo de opera��o deste computador de principal para reserva ou viceversa, se houver similar.
         RETORNO:
            TRUE: modo alterado.
            FALSE: n�o h� similar.
*/

static inline char HostConect(void) {
	//TODO: implementar host connect
	printf("TODO: check if host is connected\n");
	return 0;
}
/*
         Informa se este computador est� conectado a rede.
         RETORNO:
            TRUE: est� conectado.
            FALSE: est� isolado.
*/


static inline unsigned long DualAddress(void){
	//TODO: implementar verificação
	printf("TODO: check if is DUAL Address\n");
	return 0;
}
/*  
    FUNCAO: informa o endere�o IP da esta��o dual, se houver.
    PARAMETROS:
    RETORNO:
         = 0: n�o h� esta��o dual ou protocolo IP ausente;                                                               
         <>0: endere�o IP estruturado. 
*/

#define localhost 0X7F000001L             // valor refente ao endere�o local
extern unsigned long broadcast;             // valor refente ao endere�o broadcast 

#define  SOCK_TYPE      0XC0                // mascara p/tipo do soquete (ver acima)
#define  SOCK_BIND      0X20                // mascara p/indicador de soquete iniciado

#define  MAX_SOCK       256                	// numero maximo de soquetes no sistema

typedef struct                              // defini��o de soquete de rede
   {
   short          FAMILY;					//  familia
   unsigned short PORT;						//  numero da porta para transmitir ou receber
   unsigned long  ADDR;						//  endere�o de rede para transmitir (se !=0) ou receber (mascarado)
   unsigned char  PID;						//  identificador do processo que trata a recep��o. Se =0, n�o recebe.
   unsigned char  MASC;						//  define a mascara do endere�o na subrede. Bits em 1, diferen�a significativa.
   } sockaddr;

typedef struct                              // defini��o espec�fica do soquete 
   {
   char           STAT;						// indica��es do procedimento ligado ao soquete
   unsigned char  PID;						// identifica��o do processo proprietario
   unsigned       PORT;						// numero da porta (formato NET)
   unsigned long  ADDR;       				// endere�o IP do destino (formato NET)
   unsigned long  MASC;       				// endere�o IP do destino (formato NET)
   } socket_xdos;

typedef struct                              
   {                                        // unidade da tabela de convers�o de soquetes de recep��o
   semaforo 	ACESSO;						// exclus�o mutua de acesso
   int			TSOCKS;						// numero de soquetes ativos
   socket_xdos	SOCKETS[MAX_SOCK];    		// tabela de soquetes
   } ctrl_sock;                             		


#define AlocMem malloc
/*
         Aloca uma quantidade de bytes de preferencialmente no proprio segmento de dados.
         PAR�METROS:
         BYTES: quantidades de bytes requerida;
         RETORNO: ponteiro para memoria alocada;
                  = NULL: n�o h� mem�ria dispon�vel para quatidade solicitada.                
*/


//                                               MODULO Time.c

typedef  struct           
   {                                   			// formato da etiqueta de tempo
   unsigned short 	ANO;               			//  ano (1980 a 2099)               
   unsigned char  	MES;               			//  mes (1 a 12)                     
   unsigned char  	DIA;               			//  dia: bits 0..4: no mes (1 a 31); bits 5..7: na semana (0=inv, 1= domingo ..)                     
   unsigned char  	HORA;              			//  hora (0 a 23)                    
   unsigned char  	MINUTO;            			//  minuto (0 a 59)                  
   unsigned short 	MSEGS;             			//  milisegundo no minuto(0 a 59999) 
   } st_time;

static inline void GetTime(st_time * TAG)
{
	struct timeval tv;
	struct tm * curtm;
	gettimeofday(&tv, NULL);
	curtm = localtime(&(tv.tv_sec));
	TAG->DIA = curtm->tm_mday;
	TAG->MES = curtm->tm_mon + 1;
	TAG->ANO = curtm->tm_year;
	TAG->HORA = curtm->tm_hour,
	TAG->MINUTO = curtm->tm_min;
	TAG->MSEGS = tv.tv_usec*1000;
}
/*
         Obtem a data e hora do sistema operacional;
         PARAMETROS:
         TAG: ponteiro para area p/armazenar o tempo.
*/

static inline void SetTime(st_time * TAG)
{
	struct timeval tv;
	struct tm curtm;
	curtm.tm_mday = TAG->DIA;
	curtm.tm_mon = TAG->MES - 1;
	curtm.tm_year = TAG->ANO;
	curtm.tm_hour = TAG->HORA,
	curtm.tm_min = TAG->HORA;
	tv.tv_sec = mktime(&curtm);
	tv.tv_usec = TAG->MSEGS/1000;
	settimeofday(&tv, NULL);
}
/*
         Acerta a data e hora da esta��o;
         PARAMETROS:
         TAG: ponteiro para area com o novo valor de data e hora.
*/



//                                               MODULO SalvaMsg.c

int SalvaMsg(void * MSG);
/*
         Salva o conte�do de um buffer de memoria em disco, acrescentando ao final do arquivo MSG.BIN, 
         do diretorio corrente, incluindo o tempo em que ocorreu a opera��o.
         PAR�METROS:
         MSG: ponteiro para o buffer de memoria, que deve ter o seguinte formato:
              2 bytes: inteiro com o n�mero de bytes a seguir;
              demais bytes: formato livre.                                                     
*/ 

void SalvaBuffer(char* ARQUIVO, void * BUFFER, st_time * TIME, int TAM, bool SAINDO);
/*
         FUNCAO: salvar em disco uma sequencia de bytes com informa��o de tempo.
         PARAMETRO:
         	ARQUIVO: nome do arquivo p/ salvar buffer;
            BUFFER: ponteiro para o vetor de bytes;
            TIME: ponteiro para o tempo no formato do sistema. Se = NULL, busca tempo na hora de gravar;
            TAM: numero de bytes;
            SAINDO: indicador do sentido da comunica��o (=TRUE: transmitido)
         RETORNO:
*/


//                                               MODULO Lista.c

typedef struct                         	
   {                                   			// formato de lista de eventos
   unsigned        	NBYTES;              		//  tamanho total em bytes da lista 
   semaforo       	ACESSO;              		//  semaforo para garantir exclusividade de acesso
   int            	MAXIMO;              		//  numero maximo de eventos na lista
   int            	TAMANHO;             		//  numero de eventos armazenados na lista
   int            	INICIO;              		//  indice do primeiro evento da lista
   int            	FIM;                 		//  indice da primeira posi��o livre apos �ltimo evento                                   
   int            	LARGURA;              		//  numero de bytes de cada evento                                 
   char           	EVENTOS[1];  	      		//  vetor de eventos como lista circular                                  
   } lista;                         	
                                       	
lista * CriaLista(int MAXIMO, int LARGURA);
/*   
         Cria uma estrutura de lista.
         PARAMETROS:
         MAXIMO: n�mero maximo de eventos a serem armazenados;
         RETORNO: ponteiro para a lista. Se NULL, n�o h� mem�ria dispon�vel.                      
*/

int ExtingueLista(lista * LISTA);
/*   
         Extinguir uma estrutura de lista criada.
         PARAMETROS:
         LISTA: ponteiro para a lista;
         RETORNO: 
            = 0: LISTA n�o reconhcida;
            = 1: LISTA extinta;                      
*/

int IniciaLista(lista * LISTA, void * EVENTOS, int NEVENTOS);
/*   
         Inicia uma lista com uma lista de eventos, se fornecida.
         PARAMETROS:
         LISTA: ponteiro para a lista;
         EVENTOS: vetor de eventos fornecidos;
         NEVENTOS: numero de eventos fornecidos;
         RETORNO: n�mero de eventos na lista.                                                                         
*/

int TamanhoLista(lista * LISTA);
/*
         Devolve o n�mero de elementos da lista.
         PARAMETROS:
         LISTA: ponteiro para a lista;
         RETORNO: n�mero de eventos na lista.                  
*/

int IncluiEventoLista(lista * LISTA, void * EVENTO);
/*
         Inclui um evento numa lista.
         PARAMETROS:
         LISTA: ponteiro para a lista;
         EVENTO: evento a incluir;
         RETORNO: n�mero de eventos na lista.                 
*/

int LeEventosLista(lista * LISTA, void * EVENTOS, short MAXIMO, short INICIO);
/*
         Fornece eventos de uma lista, marcando-os como eventos lidos.
         PARAMETROS:
         LISTA: ponteiro para a lista;
         EVENTOS: ponteiro para eventos fornecidos;
         MAXIMO: numero maximo de eventos a serem fornecidos;
         INICIO: deslocamento inicial para partir a leitura (positivo: inicio da lista; negativo: fim da lista); 
         RETORNO: n�mero de eventos fornecidos.                                                   
*/

int ExcluiEventosLista(lista * LISTA, int NEVENTOS);
/*
         Exclui um numero maximo de eventos de uma lista marcados como eventos lidos.
         PARAMETROS:
         LISTA: ponteiro para a lista;
         NEVENTOS: numero de eventos a excluir;
         RETORNO: n�mero de eventos excluidos.
*/

int SalvaLista(lista * LISTA, char* ARQUIVO);
/*
         Salva uma lista num arquivo.
         PARAMETROS:
         LISTA: ponteiro para a lista;
         ARQUIVO: nome do arquivo;
         RETORNO:
            0 = erro em opera��o em disco;
            1 = lista salva em arquivo do disco.
*/

int RecuperaLista(lista * LISTA, char* ARQUIVO);
/*
         Recupera uma lista de um arquivo.
         PARAMETROS:
         LISTA: ponteiro para a lista;
         ARQUIVO: nome do arquivo;
         RETORNO:
            0 = erro em opera��o em disco;
            1 = lista lida do arquivo em disco.
*/


//                                              MODULO TFTP_APL.C

#define		TFTP_OK			0					// status de opera��o: sucesso
#define		TFTP_ERR_EXIST	1					// status de opera��o: ERRO, nome do arquivo destino j� existe
#define		TFTP_ERR_AUSNT	2					// status de opera��o: ERRO, nome do arquivo fonte n�o existe
#define		TFTP_ERR_READ	3					// status de opera��o: ERRO, em opera��o de leitura
#define		TFTP_ERR_WRITE	4					// status de opera��o: ERRO, em opera��o de escrita
#define		TFTP_ERR_TEMPO	5					// status de opera��o: ERRO, esgotado tempo de espera
#define		TFTP_ERR_REMOT	6					// status de opera��o: ERRO, em opera��o do arquivo remoto
#define		TFTP_ERR_OCUP	7					// status de opera��o: ERRO, conexao ocupada
#define		TFTP_ERR_FORA	8					// status de opera��o: ERRO, servi�o TFTP ausente
#define		TFTP_INDEFINID	9					// status de opera��o: resultado indefinido
#define		TFTP_TRANSF		10					// status de opera��o: transferencia em andamento

int TFTP_Get(unsigned long HOST, string ORIGEM, string DESTINO);
/*
	FUNCAO: Transferir um arquivo de uma m�quina remota para esta m�quina.
    PARAMETROS:
        HOST: endere�o IP da maquina remota;
        ORIGEM: nome completo do arquivo na m�quina remota;
        DESTINO: nome completo do arquivo na m�quina local;
    RETORNO: resultado da transfer�ncia:
        = 0: requisi�ao entregue ao protocolo;
        TFTP_ERR_FORA:	Servico TFTP ausente;        
*/

int TFTP_Put(unsigned long HOST, string ORIGEM, string DESTINO);
/*
	FUNCAO: Transferir um arquivo desta m�quina para uma m�quina remota.
    PARAMETROS:
        HOST: endere�o IP da maquina remota;
        ORIGEM: nome completo do arquivo na m�quina local;
        DESTINO: nome completo do arquivo na m�quina remota;
    RETORNO: resultado da transfer�ncia:
        = 0: requisi�ao entregue ao protocolo;
        TFTP_ERR_FORA:	Servico TFTP ausente;        
*/

int TFTP_Status(unsigned ESPERA);
/*
	FUNCAO: Obter o status da transferencia disparada.
    PARAMETROS:
        ESPERA: tempo de espera, em segundos, pelo t�rmino da transfer�ncia. Se = 0, espera definida pelo protocolo; 
    RETORNO: resultado da transfer�ncia:
        TFTP_OK:		Transferencia completada com exito;
        TFTP_ERR_EXIST: Arquivo destino existente;
        TFTP_ERR_AUSNT:	Arquivo origem ausente;
        TFTP_ERR_READ:	Erro de leitura no arquivo local;
        TFTP_ERR_WRITE:	Erro de escrita no arquivo local;
        TFTP_ERR_TEMPO:	Esgotado tempo de transferencia;
        TFTP_ERR_REMOT:	Erro de acesso no arquivo remoto;
        TFTP_ERR_OCUP:	Servico TFTP ocupado;
        TFTP_ERR_FORA:	Servico TFTP ausente;        
        TFTP_INDEFINID:	Resultado indefinido;        
*/


//                                              MODULO IP_APL.C

int IP_Send(unsigned long HOST, void * BUFFER, unsigned char PROTOCOLO); 
/*  
    FUNCAO: entrega um pacote para ser transmitido pelo protocolo IP.
    PARAMETROS:
         HOST: endere�o de rede da m�quina destino (formato NET);
         BUFFER: ponteiro p/a estrutura contendo numero de bytes a serem enviados nos 2 primeiros bytes;
         PROTOCOLO: identificador do protocolo usuario;
            =  1: controle ICMP
            = 17: transporte UDP;
            =  6: transporte TCP;
    RETORNO:
         0: sucesso;
         1: erro. protocolo IP ausente;                                                               
         2: erro. falha na entrega ao IP;                                                               
*/

int IP_Identif(unsigned long HOST); 
/*  
    FUNCAO: disparar um pacote de identifica��o um endere� IP.
    PARAMETROS:
         HOST: endere�o de rede da m�quina destino (formato HOST);
    RETORNO:
         0: sucesso;
         1: erro. protocolo IP ausente;                                                               
         2: erro. falha na entrega ao IP;                                                               
         3: erro. Falha na aloca��o de memoria;                                                               
*/

int UDP_Send(unsigned long IP_DEST, unsigned PORT_DEST, unsigned PORT_ORIG, buffer * BUFFER, unsigned char MULTICAST); 
/*  
    FUNCAO: formata o pacote com UDP e entrega para ser transmitido pelo protocolo IP.
    PARAMETROS:
         IP_DEST: endere�o de rede da m�quina destino (formato NET);
         PORT_DEST: numero da porta na m�quina destino (formato NET);
         PORT_ORIG: numero da porta na m�quina local (formato NET);
         BUFFER: ponteiro p/a estrutura contendo numero de bytes a serem enviados nos 2 primeiros bytes;
         MULTICAST: se TRUE, endere�o destino � "multicast";
    RETORNO:
         0: sucesso;
         1: erro. protocolo IP ausente;                                                               
         2: erro. Falha na entrega ao prtocolo IP;                                                               
         3: erro. Falha na aloca��o de memorio;                                                               
*/

int ETH_SendToIP(unsigned long IP_DEST, buffer * BUFFER); 
/*  
    FUNCAO: transmite dados encapsulados num quadro ETHERNET 802.3 para um destino identificado por seu endere�o IP.
    PARAMETROS:
         IP_DEST: endere�o de rede da m�quina destino (formato NET);
         BUFFER: ponteiro p/a estrutura contendo numero de bytes a serem enviados nos 2 primeiros bytes;
    RETORNO:
         0: sucesso;
         1: erro. protocolo IP ausente;                                                               
         2: erro. falha na entrega ao IP;                                                               
         3: erro. Falha na aloca��o de memorio;                                                               
*/


//                                              MODULO UDP_APL.C

ctrl_sock * UDP_GetSockets(void);
/*  
    FUNCAO: fornecer a localiza��o da estrutura de soquetes do sistema.
    PARAMETROS:
    RETORNO:
         <>NULL: ponteiro para area de soqutes do sistema;
         = NULL: Protocolo IP ausente;                                                               
*/

//                                              MODULO TCP_APL.C

#define 		TCP_MSS			1460            // numero m�ximo de bytes de dados num pacote TCP
#define 		TCP_MSL			120             // tempo m�ximo em segundos, de um segmento presente na rede
                                                // codigos das sinaliza��es sentido TCP -> usuario
#define 		TCP_ERROR		0               //  erro  
#define 		TCP_DATA		1               //  entrega de dados  
#define 		TCP_OPENING		2               //  status: conex�o abrindo
#define 		TCP_CONECT		3               //  status: conex�o pronta para troca de dados  
#define 		TCP_CLOSING		4               //  status: conex�o fechando 
#define 		TCP_CLOSE		5               //  status: conex�o fechada  
#define 		USER_FUNC		128             //  inicio da faixa reservada a codigos do usuario  

typedef struct
   {                                        	// mensagem trocada entre usuario e TCP
   word			SIZE;							//  numero de bytes da mensagem
   byte 		FUNC;							//  fun��o do TCP
   char 		CONEX;							//  numero da conex�o 
   byte         DATA[TCP_MSS];                	//  buffer de dados do usuario (tamanho variavel) 
   } tcp_msg;


static inline int TCP_Open(unsigned short PORTA_LOCAL, unsigned short PORTA_REMOTA, unsigned long IP_REMOTO, unsigned char ACTIVE, int server)
{
	int socketfd;
	struct sockaddr_in servSock;

	if(server) { 
		struct sockaddr_in clientSock;
		int clientSockFd;
		unsigned int clientSockLen;


		/*Create a TCP socket for incoming connections*/
		if ( (socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) ) < 0 ) {
			printf("Error creating a tcp socket \n");
			return -1;
		}

		memset(&servSock, 0, sizeof(servSock));

		servSock.sin_family = AF_INET;
		servSock.sin_port = htons(PORTA_LOCAL);
		servSock.sin_addr.s_addr = htonl(INADDR_ANY);

		/* 2) Assign a port to a socket(bind)*/
		if ( bind(socketfd, (const struct sockaddr *)&servSock, sizeof(servSock) ) < 0) {
			close(socketfd);
			fprintf(stderr, "Error on bind tcp port %d.. %s\r\n", PORTA_LOCAL, strerror(errno));
			return -1;
		}

		/*3) Set socket to listen*/
		if ( listen(socketfd, sizeof(tcp_msg)) < 0) {
			close(socketfd);
			fprintf(stderr, "Error on socket listen..\r\n");
			return -1;
		}


		printf("TCP socket initialization for port %d , address %lx, fd %d\n", PORTA_LOCAL, IP_REMOTO, socketfd);

		clientSockLen = sizeof(clientSock);

		/*wait for a client to connect*/
		if( (clientSockFd = accept(socketfd, (struct sockaddr *)&clientSock, &clientSockLen) ) < 0 ) {
			printf("Error accepting connection\n");
			close(clientSockFd);
			return -1;
		}
		return clientSockFd;
	} else {
		/*1) Create a reliable, stream socket using TCP*/
		if ( (socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) ) < 0 ) {
			fprintf(stderr, "Error creating tcp socket. Aborting!\r\n");
			return -1;
		}
		servSock.sin_family = AF_INET;
		servSock.sin_port = htons(PORTA_REMOTA);
		servSock.sin_addr.s_addr = htonl(IP_REMOTO);

		/*2) Establish connection*/
		if ( connect(socketfd, (const struct sockaddr *)&servSock, sizeof(servSock)) < 0) {
			fprintf(stderr, "Error connecting!\r\n");
			return -1;
		}
		return socketfd;
	}
	return -1;
}
/*  
    FUNCAO: abrir uma conex�o TCP.
    PARAMETROS:
    	 PORTA_LOCAL: numero da porta local (formato HOST);
    	 PORTA_REMOTA: numero da porta remota (formato HOST);
         IP_REMOTO: endere�o de rede da m�quina destino (formato HOST);
         ACTIVE: se TRUE, abertura ATIVA. Caso contrario, abertura passiva;
         USER: identificador do processo receptor de sinaliza��es do TCP. Se = 0, sem sinaliza��o (opera��o por polling);
    RETORNO:
         >= 0: identificador da conex�o (deve ser usado nas outras fun��es);
         <  0: erro:
         		= -1: protocolo TCP ausente;
         		= -2: falta momentanea de memoria do sistema operacional;
         		= -3: protocolo TCP n�o respondeu;
         		= -4: tentativa de abertura ativa de um soquete remoto indefinido;
         		= -5: todas conex�es ocupadas;
         		= -6: conex�o j� est� operando;
         		= -7: nova abertura em soquete remoto indefinido;
         		= -8: resposta inv�lida do TCP;                                                               
*/

static inline short TCP_Send(unsigned int N, void * MSG, int SIZE) {
	return send(N, MSG, SIZE, 0);
}
/*  
    FUNCAO: entregar um pacote de dados para ser transmitido por uma conex�o TCP.
    PARAMETROS:
    	 N: numero da conex�o;
    	 MSG: ponteiro para o buffer de dados;
         SIZE: numero de bytes a transmitir.
    RETORNO:
         > 0: sucesso:
               n�mero de pacotes enfileirados para transmitir;
         < 0: erro:
         		= -1: protocolo TCP ausente;
         		= -2: falta momentanea de memoria do sistema operacional;
         		= -3: conexao invalida;
         		= -4: conex�o nao ativa;
         		= -5: TCP impedido de enviar pacote com o tamanho fornecido;
         		= -6: numero excessivo de pacotes enfileirados nesta conex�o do TCP;
*/

static inline tcp_msg * TCP_Receive(int sockFd, unsigned ESPERA) {


	return WaitT(sockFd, ESPERA);
}
/*  
    FUNCAO: esperar informa��o do TCP, dados ou sinaliza��o. 
            Deve ser chamada pelo processo cadastrado como usu�rio, na abertura da conex�o.
            A informa��o pode ser de qualquer conex�o, pois um processo pode ser usu�rio de m�ltiplas conex�es. 
    PARAMETROS:
    	 ESPERA: tempo de espera por chegada de pacote (em ticks). Se = 0, espera at� chegar;
    RETORNO:
    	 <> NULL: ponteiro para um buffer do SO (deve ser liberado ap�s o uso) no formato de interface USUARIO/TCP:
    	 		SIZE: numero de bytes do buffer fornecido (unsigned);
    	 		FUNC: especifica o conte�do do buffer (byte):
    	 			TCP_DATA: buffer cont�m dados;
    	 			TCP_CONECT: pronto para trocar dados com TCP remoto;
    	 			TCP_CLOSING: TCP remoto solicitou desconex�o;
    	 			TCP_CLOSE: desconectado;
    	 			TCP_ERROR: erro de acesso ao TCP;
    	 		CONEX: numero da conex�o (byte);
    	 		DATA:  vetor de dados (tamanho variavel: SIZE-2);
    	 =  NULL, n�o chegou mensagem no tempo especificado.
*/

static inline short TCP_Close(unsigned int N) 
{
	return close(N);
}
/*  
    FUNCAO: fechar uma conex�o TCP.
    PARAMETROS:
    	 N: numero da conex�o;
    RETORNO:
         = 0: disparado o ciclo de fechamento de conex�o;
         < 0: erro:
         		= -1: protocolo TCP ausente;
         		= -2: falta momentanea de memoria do sistema operacional;
         		= -3: conexao invalida;
         		= -4: conex�o fechando;
*/

//short TCP_Status(unsigned N);
/*  
    FUNCAO: informar o estado de uma conex�o TCP.
    PARAMETROS:
    	 N: numero da conex�o;
    RETORNO: estado da conex�o:
         TCP_ERROR: protocolo TCP ausente ou conexao invalida;
         TCP_OPENING: conectando;
         TCP_CONECT: conectada;
         TCP_CLOSING: desconectando;
         TCP_CLOSE: desconectada.
*/

static inline short TCP_Abort(unsigned N){
	return close(N);
}
/*  
    FUNCAO: abortar uma conex�o TCP.
    PARAMETROS:
    	 N: numero da conex�o;
    RETORNO:
         = 0: disparado o ciclo para abortar a conex�o;
         < 0: erro:
         		= -1: protocolo TCP ausente;
         		= -2: falta momentanea de memoria do sistema operacional;
         		= -3: conexao invalida;
         		= -4: conex�o fechada;
*/

#endif
