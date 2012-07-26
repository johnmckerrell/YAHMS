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

#include "YAHMS_Config.h"
#include "YAHMS_Controller.h"

#include <Flash.h>

#ifdef LOGGING
FLASH_STRING(BLANK," ");
FLASH_STRING(COMMA,",");
FLASH_STRING(PROG_ON,"ON");
FLASH_STRING(PROG_OFF,"OFF");
FLASH_STRING(DOWNLOADING_CONFIG_FROM,"Downloading config from http://");
FLASH_STRING(HTTP_REQUEST_FAILED,"HTTP request failed.  Reported:");
FLASH_STRING(HTTP_NOT_CONNECTED,"HTTP request couldn't connect\n");
FLASH_STRING(VALID_EEPROM_CONTENTS,"Valid eeprom contents\n");
FLASH_STRING(CONFIG_SIZE_IS,"Config size is ");
FLASH_STRING(XBEE_PINS_CHANGED_TO,"Xbee pins changed to RX=");
FLASH_STRING(XBEE_PINS_TX," TX=");
FLASH_STRING(ANALOG_PINS_ARE,"Analog pins are: ");
FLASH_STRING(INPUT_PINS_ARE,"Input pins are: ");
FLASH_STRING(OUTPUT_PINS_ARE,"Output pins are: ");
FLASH_STRING(CONTROL_BLOCKS,"Control Blocks:\n");
#endif

ControlBlock *firstControlBlock = NULL, *lastControlBlock = NULL;

boolean configPresent = false;
unsigned long int configTime = 0;

char outputPins[NUM_OUTPUT_PINS];
char inputPins[NUM_INPUT_PINS];
char analogPins[NUM_ANALOG_PINS];

char xbeeRX = -1, xbeeTX = -1;

byte settings[NUM_SETTINGS];

void LogControlBlock(ControlBlock *currentControlBlock) {
  #ifdef LOGGING
  Serial.print(currentControlBlock->minute,DEC);
  BLANK.print(Serial);
  Serial.print(currentControlBlock->hour,DEC);
  BLANK.print(Serial);
  Serial.print(currentControlBlock->day,DEC);
  BLANK.print(Serial);
  Serial.print(currentControlBlock->month,DEC);
  BLANK.print(Serial);
  Serial.print(currentControlBlock->weekday,DEC);
  BLANK.print(Serial);
  Serial.print(currentControlBlock->len,DEC);
  BLANK.print(Serial);
  Serial.print(currentControlBlock->pin,DEC);
  BLANK.print(Serial);
  if (currentControlBlock->state) {
    PROG_ON.print(Serial);
  } else {
    PROG_OFF.print(Serial);
  }
  Serial.println();
  #endif
}

void CheckAndUpdateConfig() {
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
  if (!configPresent || (now()-configTime) > CONFIG_REQUEST_FREQUENCY) {
    char eepromHeader[5] = { 'Y', 'A', 'H', 'M', 'S' };
    boolean eepromValid = true, eepromRecent = true;
    byte i = 0;
    while (eepromValid) {
      if (i < 5 && eepromHeader[i] != EEPROM.read(i)) {
        eepromValid = false;
      } else if (i<9) {
        configTime = (configTime * 256) + EEPROM.read(i);
      } else {
        break;
      }
      ++i;
    }
    // Force a download by uncommenting the following line
    configTime = 0;
    if (eepromValid) {
      // If the config is over 5 mins old, refresh it
      if ((now()-configTime) > 300) {
        eepromRecent = false;
      }
    } else {
      eepromRecent = false;
    }
    // Download the config file from http://bubblino.com/<MAC Address>
    EthernetClient c;
    HttpClient http(c);
    if (!eepromRecent) {
      char configPath[31] = "/api/c/";
      //configPath[0] = '/api/c/';
      const char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
      // Append the MAC address
      for (int i =0; i < 6; i++)
      {
        configPath[7+(2*i)] = hexDigits[(mac[i] >> 4) & 0x0f];
        configPath[8+(2*i)] = hexDigits[mac[i] & 0x0f];
      }
      // And now the config version number
      configPath[19] = '/';
      configPath[20] = hexDigits[YAHMS_API_VERSION];
      // And finally append the timestamp for when we last updated the config
      configPath[21] = '/';
      for (int i =0; i<4; ++i)
      {
        unsigned long int timeDigit = configTime >> (8 * (3-i));
        configPath[22+(2*i)] = hexDigits[(timeDigit >> 4) & 0x0f];
        configPath[23+(2*i)] = hexDigits[timeDigit & 0x0f];
      }
      configPath[30] = '\0';
  
      #ifdef LOGGING
      DOWNLOADING_CONFIG_FROM.print(Serial);
      Serial.print(YAHMS_SERVER);
      Serial.println(configPath);
      #endif
      if (http.get(YAHMS_SERVER, 80, configPath, USERAGENT) == 0)
      {
        // Request sent okay
        
        // Look for the response, and specifically pull out the content-length header
        unsigned long timeoutStart = millis();
        char* current;
  
        int err = http.responseStatusCode();
        if ((err >= 200) && (err < 300))
        {
          // We've got a successful response
          // And we're not interested in any of the headers
          err = http.skipResponseHeaders();
          if (err >= 0)
          {
            // We're onto the body now
  
            // Store the config in EEPROM, in the format:
            // 00-04: IP address of remote server (e.g. search.twitter.com)
            // 05   : Length of following remote server name
            // 06-  : remote server name (e.g. string "search.twitter.com")
            //      : Length of following path, etc. in URL
            //      : path, etc. for URL
            unsigned long int i =0;
            char c = '\0';
            while ( (i < http.contentLength()) && (i < 512) && ( (millis() - timeoutStart) < (RESPONSETIMEOUT*1000) ))
            {
              if (http.available())
              {
                // We've got some data to read
                c = http.read();
                if (i < 5) {
                  if (c != eepromHeader[i]) {
                    break;
                  }
                  if (i == 4) {
                    configTime = now();
                    for( int j = 0; j < 5; ++j ) {
                      EEPROM.write(j,eepromHeader[j]);
                    }
                    unsigned long outputTime = configTime;
                    // Write out digits from right to left
                    for (int j =3; j >= 0; --j)
                    {
                      unsigned long int timeDigit = outputTime & 0xFF;
                      outputTime /= 256;
                      EEPROM.write(j+5,timeDigit);
                    }

                  }
                } else {
                  // Store it in the EEPROM
                  // Leaving a gap to store the size
                  // But remember that i will be 5 when we start writing
                  // the EEPROM start position needs to be 5 + 4 + 4
                  EEPROM.write(8 + i, c);
                  // And reset the timeout counter
                  timeoutStart = millis();
                }
                ++i;
              }
              else
              {
                // We haven't got any data, so lets pause to allow some to arrive
                delay(1000);
              }
            }
            if (i > 0) {
              // Add a new-line to make sure we parse all the numbers
              EEPROM.write(13 + i, '\n');
              ++i;
              for (int j = 3; j >= 0; --j) {
                unsigned long int sizeDigit = i & 0xFF;
                i /= 256;
                EEPROM.write(j+9,sizeDigit);
              }
            }
            eepromValid = true;
          }
        }
        else
        {
          // This isn't a successful response
  #ifdef LOGGING
          HTTP_REQUEST_FAILED.print(Serial);
          Serial.println(err);
  #endif
        }
        
        http.stop();
        c.stop();
      }
  #ifdef LOGGING
      else
      {
        // else hope that the last config was okay
        HTTP_NOT_CONNECTED.print(Serial);
      }
  #endif
    }
    if (eepromValid) {
      configTime = now();
      #ifdef LOGGING
      VALID_EEPROM_CONTENTS.print(Serial);
      #endif
      configPresent = true;
      unsigned long int configSize= 0;
      for (int i = 9; i < 13; ++i) {
        configSize = (256 * configSize) + EEPROM.read(i);
      }
      #ifdef LOGGING
      CONFIG_SIZE_IS.print(Serial);
      Serial.println(configSize);
      #endif
      ControlBlock *currentControlBlock = NULL;
      while (firstControlBlock) {
        currentControlBlock = firstControlBlock;
        firstControlBlock = firstControlBlock->next;
        free(currentControlBlock);
      }
      lastControlBlock = NULL;
      
      // Now parse the EEPROM contents line by line
      int i = 0;
      boolean xbeeChanged = false;
      currentControlBlock = NULL;
      int controlBlockValues[8];
      boolean startedControlBlock = false;
      int intVal = -1;
      int intIndex = 0;
      byte c = '\0';
      char lineMode = '\0';
      while (i < configSize+13) {
        c = EEPROM.read(i);
        if (i < 13) {
          // Ignore it
        } else if (isdigit(c)) {
          if (lineMode == 'C' && !startedControlBlock) {
            memset(controlBlockValues,-1,sizeof(controlBlockValues));
            startedControlBlock = true;
            //Serial.println("Initialising controlBlockValues");
          }
          if (intVal == -1)
            intVal = 0;
          intVal = (10*intVal) + (c - '0');
        } else if (lineMode == '\0') {
          //Serial.print("lineMode=");
          lineMode = c;
          //Serial.println(lineMode);
          intVal = -1;
          intIndex = 0;
        } else if (c == '*' && lineMode == 'C') {
            if (intIndex < sizeof(controlBlockValues)) {
              controlBlockValues[intIndex] = -1;
            }
            intVal = -1;
            ++intIndex;
        } else if (intVal != -1) {
          switch(lineMode) {
            case 'S':
              if (intIndex < NUM_SETTINGS) {
                settings[intIndex] = intVal;
              }
              break;
            case 'A':
              // Analog pins:
              if (intIndex < NUM_ANALOG_PINS) {
                analogPins[intIndex] = intVal;
              }
              break;
            case 'I':
              // Digital pins to set as input
              if (intIndex < NUM_INPUT_PINS) {
                inputPins[intIndex] = intVal;
              }
              break;
            case 'O':
              // Digital pins set as output
              if (intIndex < NUM_OUTPUT_PINS) {
                outputPins[intIndex] = intVal;
              }
              break;
            case 'X':
              // Pins to use for Xbee
              if (intIndex == 0 && xbeeRX != intVal) {
                xbeeChanged = true;
                xbeeRX = intVal;
              } else if (intIndex == 1 && xbeeTX != intVal) {
                xbeeChanged = true;
                xbeeTX = intVal;
              }
              break;
            case 'C':
              if (intIndex < sizeof(controlBlockValues)) {
                /*
                Serial.print("Storing controlBlockValues[");
                Serial.print(intIndex);
                Serial.print("]=");
                Serial.println(intVal);
                */
                controlBlockValues[intIndex] = intVal;
              }
              break;
          }
          intVal = -1;
          ++intIndex;
        }
        if (c == '\n' || c == '\r' ) {
          switch(lineMode) {
            case 'S':
              for(int i = intIndex; i < NUM_SETTINGS; ++i) {
                settings[i] = 0;
              }
              break;
            case 'A':
              for(int i = intIndex; i < NUM_ANALOG_PINS; ++i) {
                analogPins[i] = -1;
              }
              break;
            case 'I':
              for(int i = intIndex; i < NUM_INPUT_PINS; ++i) {
                inputPins[i] = -1;
              }
              break;
            case 'O':
              for(int i = intIndex; i < NUM_OUTPUT_PINS; ++i) {
                outputPins[i] = -1;
              }
              break;
            case 'C':
              // Check that the length of the time block is greater than zero
              // and that the pin that's being turned on is zero or more
              if (controlBlockValues[5] > 0 && controlBlockValues[6] >= 0) {
                currentControlBlock = (ControlBlock*)malloc(sizeof(ControlBlock));
                // Copy the integer values into a ControlBlock
                currentControlBlock->minute = controlBlockValues[0];                
                currentControlBlock->hour = controlBlockValues[1];                
                currentControlBlock->day = controlBlockValues[2];                
                currentControlBlock->month = controlBlockValues[3];                
                currentControlBlock->weekday = controlBlockValues[4];                
                currentControlBlock->len = controlBlockValues[5];                
                currentControlBlock->pin = controlBlockValues[6];    
                currentControlBlock->state = controlBlockValues[7];            
                if (lastControlBlock) {
                  lastControlBlock->next = currentControlBlock;
                }
                currentControlBlock->next = NULL;
                lastControlBlock = currentControlBlock;
                if (!firstControlBlock) {
                  firstControlBlock = currentControlBlock;
                }
              } else {
                /*
                Serial.println("Not storing controlBlockValues");
                Serial.print("5=");
                Serial.println(controlBlockValues[5]);
                Serial.print("6=");
                Serial.println(controlBlockValues[6]);
                */
                //Serial.println(controlBlockValues);
              }
              break;
          }
          lineMode = '\0';
          startedControlBlock = false;
        }
        ++i;
      }
      // We don't need to check whether we're part-way through a number
      // here because we add a new-line above
      

      if (xbeeChanged) {
        #ifdef LOGGING
        XBEE_PINS_CHANGED_TO.print(Serial);
        Serial.print(xbeeRX,DEC);
        XBEE_PINS_TX.print(Serial);
        Serial.println(xbeeTX,DEC);
        #endif

        SoftwareSerial *serial = new SoftwareSerial(xbeeRX,xbeeTX);        
        if (xbeeSerial)
          delete xbeeSerial;
        xbeeSerial = serial;
        xbee.setSerial(*serial);
        xbee.begin(9600);
      }
      #ifdef LOGGING
      ANALOG_PINS_ARE.print(Serial);
      for(int i = 0; i < NUM_ANALOG_PINS; ++i) {
        if (i > 0) {
          COMMA.print(Serial);
        }
        Serial.print(analogPins[i],DEC);
      }
      Serial.println();
      INPUT_PINS_ARE.print(Serial);
      #endif
      for(int i = 0; i < NUM_INPUT_PINS; ++i) {
        #ifdef LOGGING
        if (i > 0) {
          COMMA.print(Serial);
        }
        Serial.print(inputPins[i],DEC);
        #endif
        if (inputPins[i]>0) {
          pinMode(inputPins[i],INPUT);
        }
      }
      #ifdef LOGGING
      Serial.println();
      OUTPUT_PINS_ARE.print(Serial);
      #endif
      for(int i = 0; i < NUM_OUTPUT_PINS; ++i) {
        #ifdef LOGGING
        if (i > 0) {
          COMMA.print(Serial);
        }
        Serial.print(outputPins[i],DEC);
        #endif
        if (outputPins[i]>0) {
          pinMode(outputPins[i],OUTPUT);
          //currentPinState[i] = -1;
        }
      }
      #ifdef LOGGING
      Serial.println();
      #endif
      
      currentControlBlock = firstControlBlock;
      #ifdef LOGGING
      CONTROL_BLOCKS.print(Serial);
      while (currentControlBlock) {
        LogControlBlock(currentControlBlock);
        currentControlBlock = currentControlBlock->next;
      }
      #endif
    }
  }
  
}
