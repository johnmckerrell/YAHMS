#include "YAHMS_Defines.h"
#include "YAHMS_Controller.h"
#include "YAHMS_Config.h"

#include <Flash.h>

#ifdef LOGGING
FLASH_STRING(ACTIVE_CONTROL_BLOCK,"Active Control Block\n");
FLASH_STRING(SETTING_OUTPUTPINS,"Setting outputPins[");
FLASH_STRING(OUTPUT_PINS_SEP1,"] (");
FLASH_STRING(OUTPUT_PINS_SEP2,") to ");
FLASH_STRING(PROG_HIGH,"HIGH");
FLASH_STRING(PROG_LOW,"LOW");
#endif

#define CB_MINUTE   0
#define CB_HOUR     1
#define CB_DAY      2
#define CB_MONTH    3
#define CB_WEEKDAY  4
#define CB_LEN      5
#define CB_PIN      6
#define CB_STATE    7


int currentPinState[NUM_OUTPUT_PINS];

uint8_t * heapptr, * stackptr;
void check_mem() {
  stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);           // save value of stack pointer
}

void SetupController() {
  for (int i = 0; i < NUM_OUTPUT_PINS; ++i) {
    currentPinState[i] = -1;
  }
}

void CheckAndUpdateState() {
  if (!configPresent)
    return;
    
  unsigned long int t = now();
  Serial.flush();
        check_mem();
      Serial.print("heapptr=");
      Serial.println((int)heapptr,DEC);
      Serial.print("stackptr=");
      Serial.println((int)stackptr,DEC);
Serial.flush();
  // Add timezone/DST
  if (settings[TIME_OFFSET_MINS_SETTING] > 0) {
    t += settings[TIME_OFFSET_MINS_SETTING]*60;
  }
  int minsSinceMidnight = hour(t)*60+minute(t);
  for (int i = 0; i < NUM_OUTPUT_PINS; ++i) {
    //Serial.print("i=");
    //Serial.println(i);
    if (outputPins[i] == -1)
      continue;
      
    int pinState = -1;
    
    unsigned long int configSize= 0;
    int confi = 0;
    for (confi = 9; confi < 13; ++confi) {
      configSize = (256 * configSize) + EEPROM.read(confi);
    }
    Serial.print("configSize");
    Serial.println(configSize);

    int controlBlockValues[8];
    int controlBlockValuesSize = 8;
    int intVal = -1;
    int intIndex = 0;
    byte c = '\0';
    char lineMode = '\0';
    while (confi < configSize+13) {
      c = EEPROM.read(confi);
      ++confi;
      if (confi < 13) {
        // Ignore it
      } else if (isdigit(c)) {
        if (intVal == -1)
          intVal = 0;
        intVal = (10*intVal) + (c - '0');
      } else if (lineMode == '\0') {
        //Serial.print("lineMode=");
        lineMode = c;
        //Serial.println(lineMode);
        intVal = -1;
        intIndex = 0;
        if (lineMode == 'C') {
          memset(controlBlockValues,-1,sizeof(controlBlockValues));
        }
      } else if (c == '*' && lineMode == 'C') {
          if (intIndex < controlBlockValuesSize) {
            controlBlockValues[intIndex] = -1;
          }
          intVal = -1;
          ++intIndex;
      } else if (intVal != -1) {
        switch(lineMode) {
          case 'C':
            if (intIndex < controlBlockValuesSize) {
              controlBlockValues[intIndex] = intVal;
            }
            break;
        }
        intVal = -1;
        ++intIndex;
      }
      if (c == '\n' || c == '\r' ) {
        switch(lineMode) {
          case 'C':
            // Check that the length of the time block is greater than zero
            // and that the pin that's being turned on is zero or more
                                Serial.print("C:"); for (int f = 0, fl = controlBlockValuesSize; f < fl; ++f) { Serial.print(controlBlockValues[f]); Serial.print(":"); }
                    Serial.println();

            if (controlBlockValues[CB_LEN] > 0 && controlBlockValues[CB_PIN] >= 0) {
          

              if (controlBlockValues[CB_PIN] != outputPins[i]) {
                lineMode = '\0';
                continue;
              }
                
              if (controlBlockValues[CB_DAY] != -1 && controlBlockValues[CB_DAY] != day(t)) {
                lineMode = '\0';
                continue;
              }
              
              if (controlBlockValues[CB_MONTH] != -1 && controlBlockValues[CB_MONTH] != month(t)) {
                lineMode = '\0';
                continue;
              }
              
              if (controlBlockValues[CB_WEEKDAY] != -1) {
                if (controlBlockValues[CB_WEEKDAY] == 8 && (weekday(t) == 1 || weekday(t) == 7) ) {
                  // dow is valid - weekend
                } else if (controlBlockValues[CB_WEEKDAY] == 9 && weekday(t) >= 2 && weekday(t) <= 6) {
                  // dow is valid - weekday
                } else if (controlBlockValues[CB_WEEKDAY] != weekday(t)) {
                  // dow not valid, skip this one
                  lineMode = '\0';
                  continue;
                }
              }
              if (controlBlockValues[CB_HOUR] != -1) {
                if (controlBlockValues[CB_MINUTE] != -1) {
                  int mins = (controlBlockValues[CB_HOUR]*60)+controlBlockValues[CB_MINUTE];
                  if (mins <= minsSinceMidnight && (mins+controlBlockValues[CB_LEN]) > minsSinceMidnight) {
                    pinState = controlBlockValues[CB_STATE] ? 1 : 0;
                    #ifdef LOGGING
                    ACTIVE_CONTROL_BLOCK.print(Serial);
                     for (int f = 0, fl = controlBlockValuesSize; f < fl; ++f) { Serial.print(controlBlockValues[f]); Serial.print(":"); }
                    Serial.println();
                    #endif
                  }
                } else if (controlBlockValues[CB_HOUR] == hour(t)) {
                  pinState = controlBlockValues[CB_STATE] ? 1 : 0;
                  #ifdef LOGGING
                  ACTIVE_CONTROL_BLOCK.print(Serial);
                     for (int f = 0, fl = controlBlockValuesSize; f < fl; ++f) { Serial.print(controlBlockValues[f]); Serial.print(":"); }
                  Serial.println();
                  #endif
                }
              } else if (controlBlockValues[CB_MINUTE] != -1) {
                if (controlBlockValues[CB_MINUTE] <= minute(t) && (controlBlockValues[CB_MINUTE] + controlBlockValues[CB_LEN]) > minute(t)) {
                  pinState = controlBlockValues[CB_STATE] ? 1 : 0;
                  #ifdef LOGGING
                  ACTIVE_CONTROL_BLOCK.print(Serial);
                     for (int f = 0, fl = controlBlockValuesSize; f < fl; ++f) { Serial.print(controlBlockValues[f]); Serial.print(":"); }
                  Serial.println();
                  #endif
                }
              } else {
                //pinState = 0;
                //Serial.println("Turned off by control block:");
                //LogControlBlock(currentControlBlock);
              }
              if (pinState == 0) {
                // Any "off" blocks override others
                //Serial.println("Turned off by control block:");
                break;
              }
              
            }
            break;
        }
        lineMode = '\0';
        if (pinState == 0) {
          break;
        }
      }
    }

    if (pinState == -1)
      pinState = 0;
    if (pinState != currentPinState[i]) {
      #ifdef LOGGING
      SETTING_OUTPUTPINS.print(Serial);
      Serial.print(i);
      OUTPUT_PINS_SEP1.print(Serial);
      Serial.print(outputPins[i],DEC);
      OUTPUT_PINS_SEP2.print(Serial);
      if (pinState) {
        PROG_HIGH.print(Serial);
      } else {
        PROG_LOW.print(Serial);
      }
      Serial.println();
      #endif
      digitalWrite(outputPins[i], pinState ? HIGH : LOW);
      currentPinState[i] = pinState;
    }
  }
          check_mem();
      Serial.print("heapptr=");
      Serial.println((int)heapptr,DEC);
      Serial.print("stackptr=");
      Serial.println((int)stackptr,DEC);
Serial.flush();
}
