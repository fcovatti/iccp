#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

// Time in seconds for buffering an event on a dataset before reporting to the client
#define DATASET_ANALOG_BUFFER_INTERVAL 10
#define DATASET_DIGITAL_BUFFER_INTERVAL 1
#define DATASET_EVENTS_BUFFER_INTERVAL 1

// Time in seconds for Integrity check (configured on the remote server on initialization)
#define DATASET_ANALOG_INTEGRITY_TIME 120
#define DATASET_DIGITAL_INTEGRITY_TIME 180
#define DATASET_EVENTS_INTEGRITY_TIME 300

// Max size of the dataset. IMPORTANT (bigger than 600 can cause code crash)
#define DATASET_MAX_SIZE 500

// MAx number of datasets allowed on the client
#define DATASET_ANALOG_MAX_NUMBER 150

// MAx number of datasets allowed on the client
#define DATASET_DIGITAL_MAX_NUMBER 150

// MAx number of datasets allowed on the client
#define DATASET_EVENTS_MAX_NUMBER 150

// Name of the VCC on the remote server
#define IDICCP "HIS"

// Name or IP of the remote iccp server
#define SERVER_NAME_1 "ems3"
#define SERVER_NAME_2 "ems3"
#define SERVER_NAME_3 "lab-ems1"
#define SERVER_NAME_4 "lab-ems1"

// Name of the configuration file
#define CONFIG_FILE "sage_id_no_155.txt"

// Name of the configuration log file
#define CONFIG_LOG "iccp_config.log"

// Name of the data received log file
#define DATA_ANALOG_LOG "iccp_data_analog.bin"
#define DATA_DIGITAL_LOG "iccp_data_digital.bin"
#define DATA_EVENTS_LOG "iccp_data_events.bin"

// Name of the error log file
#define ERROR_LOG "iccp_error.log"

#endif
