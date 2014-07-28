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

#define ICCP_CLIENT_CONFIG_FILE "iccp_client_config.txt"

// OFFSET for the index in received reports
#define INDEX_OFFSET 3

// Size of analog data in RULE 2 Information reports
#define RULE2_ANALOG_REPORT_SIZE 7

// Size of digital data in RULE 2 Information reports 
#define RULE2_DIGITAL_REPORT_SIZE 9

// Size of analog data in RULE 0 Information reports
#define RULE0_ANALOG_REPORT_SIZE 5

// Size of digital data in RULE 0 Information reports 
#define RULE0_DIGITAL_REPORT_SIZE 7

#define MAX_ID_ICCP_NAME 10
#define MAX_SRV_NAME 20

// Max size of the dataset. IMPORTANT (bigger than 600 can cause code crash)
#define DATASET_MAX_SIZE 500

// MAx number of datasets allowed on the client
#define DATASET_ANALOG_MAX_NUMBER 150

// MAx number of datasets allowed on the client
#define DATASET_DIGITAL_MAX_NUMBER 150

// MAx number of datasets allowed on the client
#define DATASET_EVENTS_MAX_NUMBER 150

// Name of the data received log file
#define DATA_ANALOG_LOG "iccp_data_analog.bin"
#define DATA_DIGITAL_LOG "iccp_data_digital.bin"
#define DATA_EVENTS_LOG "iccp_data_events.bin"


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
