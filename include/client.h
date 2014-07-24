#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <malloc.h>
#include <signal.h>
#include "mms_client_connection.h"
#include "client.h"

// OFFSET for the index in received reports
#define INDEX_OFFSET 3

// Size of analog data in RULE 2 Information reports
#define RULE2_ANALOG_REPORT_SIZE 7

// Size of digital data in RULE 2 Information reports 
#define RULE2_DIGITAL_REPORT_SIZE 9

// name size of the standard dataset created
#define DATASET_NAME_SIZE 7

// name size of SAGE standard transferset
#define TRANSFERSET_NAME_SIZE 13

// Max Connection errors before aborting
#define MAX_CONNECTION_ERRORS 10

// Max Reading variable error in 
#define MAX_READ_ERRORS 10

typedef struct {
	unsigned int nponto;
	char id[25];
	char state_on[16];
	char state_off[16];
	char type;
	float f;
	unsigned char state;
	time_t time_stamp;
} data_config;

typedef enum{
	DATASET_ANALOG,
	DATASET_DIGITAL,
	DATASET_EVENTS,
	DATASET_COMMANDS
} DataSetTypes;

typedef struct {
	char id[DATASET_NAME_SIZE];
	char ts[TRANSFERSET_NAME_SIZE];
	DataSetTypes type;
	int size;
	unsigned int offset;
} dataset_config;

typedef struct {
	unsigned int nponto;
	float f;
	unsigned char state;
	time_t time_stamp;
} __attribute__((packed)) data_analog_out;

typedef struct {
	unsigned int nponto;
	unsigned char state;
	time_t time_stamp;
} __attribute__((packed)) data_digital_out;

typedef union {
	float f;
	char  s[4];
}float_data ;

typedef union {
	unsigned int t;
	char  s[4];
} time_data;


#endif
