#define YAHMS_API_VERSION 1
#define USERAGENT "YAHMS/0.2"
#define YAHMS_CONFIG_SERVER "live.yahms.net"
#define YAHMS_SUBMIT_SERVER "api.yahms.net"
#define YAHMS_PORT 80
#define RESPONSETIMEOUT 30
#define LOGGING

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define NUM_OUTPUT_PINS 60
#define NUM_INPUT_PINS 60
#define NUM_ANALOG_PINS 20
#else
#define NUM_OUTPUT_PINS 2
#define NUM_INPUT_PINS 1
#define NUM_ANALOG_PINS 2
#endif

#define NUM_SAMPLES 10
#define LAST_SAMPLE 1
#define SAMPLE_SUBMIT_FREQUENCY 60
#define CONFIG_REQUEST_FREQUENCY 60
#define SAMPLE_FREQUENCY 10

#define NUM_XBEE_PINS 2
#define MAX_XBEE_NODES 3

#define NUM_SETTINGS 1
#define TIME_OFFSET_MINS_SETTING 0
#define CONFIGTIME_INDEX 0
#define CURRENTTIME_INDEX 1

#include "YAHMS_Local.h"
