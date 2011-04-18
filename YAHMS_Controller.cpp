#include "YAHMS_Defines.h"
#include "YAHMS_Controller.h"
#include "YAHMS_Config.h"

#include <Flash.h>

#ifdef LOGGING
FLASH_STRING(ACTIVE_CONTROL_BLOCK,"Active Control Block");
FLASH_STRING(SETTING_OUTPUTPINS,"Setting outputPins[");
FLASH_STRING(OUTPUT_PINS_SEP1,"] (");
FLASH_STRING(OUTPUT_PINS_SEP2,") to ");
FLASH_STRING(PROG_HIGH,"HIGH");
FLASH_STRING(PROG_LOW,"LOW");
#endif

int currentPinState[NUM_OUTPUT_PINS];

void SetupController() {
  for (int i = 0; i < NUM_OUTPUT_PINS; ++i) {
    currentPinState[i] = -1;
  }
}

void CheckAndUpdateState() {
  unsigned long int t = now();
  // Add timezone/DST
  if (settings[TIME_OFFSET_MINS_SETTING] > 0) {
    t += settings[TIME_OFFSET_MINS_SETTING]*60;
  }
  int minsSinceMidnight = hour(t)*60+minute(t);
  ControlBlock *currentControlBlock = NULL;
  for (int i = 0; i < NUM_OUTPUT_PINS; ++i) {
    //Serial.print("i=");
    //Serial.println(i);
    if (outputPins[i] == -1)
      continue;
    int pinState = -1;
    for (currentControlBlock = firstControlBlock;currentControlBlock;currentControlBlock = currentControlBlock->next) {
      if (currentControlBlock->pin != outputPins[i])
        continue;
        
      if (currentControlBlock->day != -1 && currentControlBlock->day != day(t))
        continue;
      
      if (currentControlBlock->month != -1 && currentControlBlock->month != month(t))
        continue;
      
      if (currentControlBlock->weekday != -1) {
        if (currentControlBlock->weekday == 8 && (weekday(t) == 1 || weekday(t) == 7) ) {
          // dow is valid - weekend
        } else if (currentControlBlock->weekday == 9 && weekday(t) >= 2 && weekday(t) <= 6) {
          // dow is valid - weekday
        } else if (currentControlBlock->weekday != weekday(t)) {
          // dow not valid, skip this one
          continue;
        }
      }
      if (currentControlBlock->hour != -1) {
        if (currentControlBlock->minute != -1) {
          int mins = (currentControlBlock->hour*60)+currentControlBlock->minute;
          if (mins <= minsSinceMidnight && (mins+currentControlBlock->len) > minsSinceMidnight) {
            pinState = currentControlBlock->state ? 1 : 0;
            #ifdef LOGGING
            Serial.println(ACTIVE_CONTROL_BLOCK);
            LogControlBlock(currentControlBlock);
            #endif
          }
        } else if (currentControlBlock->hour == hour(t)) {
          pinState = currentControlBlock->state ? 1 : 0;
          #ifdef LOGGING
          Serial.println(ACTIVE_CONTROL_BLOCK);
          LogControlBlock(currentControlBlock);
          #endif
        }
      } else if (currentControlBlock->minute != -1) {
        if (currentControlBlock->minute <= minute(t) && (currentControlBlock->minute + currentControlBlock->len) > minute(t)) {
          pinState = currentControlBlock->state ? 1 : 0;
          #ifdef LOGGING
          Serial.println(ACTIVE_CONTROL_BLOCK);
          LogControlBlock(currentControlBlock);
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
    if (pinState == -1)
      pinState = 0;
    if (pinState != currentPinState[i]) {
      #ifdef LOGGING
      Serial.print(SETTING_OUTPUTPINS);
      Serial.print(i);
      Serial.print(OUTPUT_PINS_SEP1);
      Serial.print(outputPins[i],DEC);
      Serial.print(OUTPUT_PINS_SEP2);
      Serial.println(pinState ? PROG_HIGH : PROG_LOW);
      #endif
      digitalWrite(outputPins[i], pinState ? HIGH : LOW);
      currentPinState[i] = pinState;
    }
  }
}
