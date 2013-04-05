#include <Client.h>
#include <Ethernet.h>
#include <IPAddress.h>

#include <SoftwareSerial.h>
#include <XBee.h>

#include <Time.h>

#include <HttpClient.h>
#include <EEPROM.h>

#include <math.h>

extern int currentPinState[];
extern uint8_t * heapptr;
extern uint8_t * stackptr;

void check_mem();

void SetupController();

void CheckAndUpdateState();
