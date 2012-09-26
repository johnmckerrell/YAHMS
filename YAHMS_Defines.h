#define YAHMS_API_VERSION 1
#define USERAGENT "YAHMS/0.2"
#define YAHMS_SERVER "yahms.net"
#define RESPONSETIMEOUT 30
#define LOGGING

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define NUM_OUTPUT_PINS 60
#define NUM_INPUT_PINS 60
#define NUM_ANALOG_PINS 20
#else
#define NUM_OUTPUT_PINS 10
#define NUM_INPUT_PINS 1
#define NUM_ANALOG_PINS 6
#endif

#define NUM_SAMPLES 10
#define LAST_SAMPLE 1
#define SAMPLE_SUBMIT_FREQUENCY 60
#define CONFIG_REQUEST_FREQUENCY 60

#define NUM_XBEE_PINS 5
#define MAX_XBEE_NODES 3

#define NUM_SETTINGS 1
#define TIME_OFFSET_MINS_SETTING 0

#include "YAHMS_Local.h"
