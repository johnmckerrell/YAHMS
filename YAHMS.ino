/**
 * YAHMS - Yet Another Home Management System
 *
 * (C) John McKerrell 2011
 * Probably some open source license when I get around to choosing one
 */

/**
Do we have the current time or is it a while since we updated?
	Update the time - similar choice of sync vs async as below

Do we have any config, or is the config too old?
	Download a config
	?? Does this happen asynchronously or not? First time it might as well be sync but otherwise perhaps not
	What does config contain?
		Instructions on which digital pins are: Xbee Serial, Input, Output
		"On blocks" - blocks of time that a relay should be set to on
		+1 Hour - could simply be an extra On block
		Not sure about advance blocks - perhaps also an On block, these and +1 hour could be a special type that you only have one of, so a later +1 or advance discards the previous

Read in all of the local samples
Check the Xbee for any samples

Iterate over outputs, check whether we’re inside an On block, update pin status accordingly

Check the last time we sent back a sample, if it’s been too long then send the latest values, don’t send a sample that’s too old
*/

#include <Client.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <IPAddress.h>
#include <Server.h>
#include <Udp.h>
#include <util.h>

#include <SoftwareSerial.h>
#include <XBee.h>

#include <SPI.h>

#include <Time.h>

#include <b64.h>
#include <HttpClient.h>

#include <EEPROM.h>

#include <Flash.h>

#include "YAHMS_Defines.h"
#include "YAHMS_SyncTime.h"
#include "YAHMS_Config.h"
#include "YAHMS_Sampler.h"
#include "YAHMS_Controller.h"

#ifdef LOGGING

FLASH_STRING(CONFIGURING_ETHERNET,"\n\n!!!\n\nConfiguring ethernet\n");
FLASH_STRING(CONFIGURING_ETHERNET_FAILED,"Failed to configure Ethernet using DHCP\n");
FLASH_STRING(SYNCING_TIME,"Syncing time\n");
FLASH_STRING(SETUP_SAMPLER,"Setup sampler\n");
FLASH_STRING(SETUP_CONTROLLER,"Setup controller\n");
FLASH_STRING(CHECK_FOR_CONFIG,"Check for config\n");
FLASH_STRING(TAKE_SAMPLES,"TakeSamples\n");
FLASH_STRING(UPDATE_OUTPUT_PINS,"Update output pins\n");
FLASH_STRING(CHECK_AND_SUBMIT_SAMPLES,"CheckAndSubmitSamples\n");
#endif

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = YAHMS_LOCAL_MAC;

extern char outputPins[];
extern char inputPins[];
extern char analogPins[];

extern char xbeeRX;
extern char xbeeTX;

XBee xbee = XBee();
SoftwareSerial *xbeeSerial = NULL;

void setup() {
  Serial.begin(9600);
  
  for( int i = 0; i < NUM_OUTPUT_PINS; ++i ) {
    outputPins[i] = -1;
    inputPins[i] = -1;
    if (i<NUM_ANALOG_PINS) {
      analogPins[i] = -1;
    }
  }
  // start Ethernet and UDP
  #ifdef LOGGING
    CONFIGURING_ETHERNET.print(Serial);
  #endif
  if (Ethernet.begin(mac) == 0) {
    #ifdef LOGGING
    CONFIGURING_ETHERNET_FAILED.print(Serial);
    #endif
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  #ifdef LOGGING
  SYNCING_TIME.print(Serial);
  #endif
  SyncTime_setup();
  #ifdef LOGGING
  SETUP_SAMPLER.print(Serial);
  #endif
  SetupSampler();
  #ifdef LOGGING
  SETUP_CONTROLLER.print(Serial);
  #endif
  SetupController();
}

void loop() {
  /**
    Do we have the current time or is it a while since we updated?
    Update the time - similar choice of sync vs async as below
    -- All handled by the synctime stuff which will do the ntp when necessary
  */
  /*
  Do we have any config, or is the config too old?
	Download a config
	?? Does this happen asynchronously or not? First time it might as well be sync but otherwise perhaps not
	What does config contain?
		Instructions on which digital pins are: Xbee Serial, Input, Output
		"On blocks" - blocks of time that a relay should be set to on
		+1 Hour - could simply be an extra On block
		Not sure about advance blocks - perhaps also an On block, these and +1 hour could be a special type that you only have one of, so a later +1 or advance discards the previous
  */
  #ifdef LOGGING
  CHECK_FOR_CONFIG.print(Serial);
  #endif
  CheckAndUpdateConfig();
/*

Read in all of the local samples
Check the Xbee for any samples
*/
  #ifdef LOGGING
  TAKE_SAMPLES.print(Serial);
  #endif
  TakeSamples();
/*

Iterate over outputs, check whether we’re inside an On block, update pin status accordingly
*/
  #ifdef LOGGING
  UPDATE_OUTPUT_PINS.print(Serial);
  #endif
  CheckAndUpdateState();
/*
Check the last time we sent back a sample, if it’s been too long then send the latest values, don’t send a sample that’s too old
*/
  #ifdef LOGGING
  CHECK_AND_SUBMIT_SAMPLES.print(Serial);
  #endif
  CheckAndSubmitSamples();
  /*
  int i = 1;
  byte *a = (byte*)malloc(sizeof(byte));
  while (a) {
    Serial.println(i);
    a = NULL;
    a = (byte*)malloc(sizeof(byte));
    ++i;
  }
  */
}
