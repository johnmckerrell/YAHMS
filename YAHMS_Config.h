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
#include <Ethernet.h>
#include <IPAddress.h>

#include <NewSoftSerial.h>
#include <XBee.h>

#include <Time.h>

#include <HttpClient.h>
#include <EEPROM.h>

#include <math.h>

#include "YAHMS_Defines.h"

extern byte mac[];
extern XBee xbee;
extern NewSoftSerial *xbeeSerial;
extern char outputPins[];
extern byte settings[];

#ifndef YAHMS_CONFIG_H
#define YAHMS_CONFIG_H

struct ControlBlock {
  char minute;
  char hour;
  char day;
  char month;
  char weekday;
  int len;
  char pin;
  boolean state;
  struct ControlBlock *next;
};

typedef struct ControlBlock ControlBlock;

#endif

extern ControlBlock *firstControlBlock;

void LogControlBlock(ControlBlock *currentControlBlock);
void CheckAndUpdateConfig();
