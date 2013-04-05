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

#include "YAHMS_Sampler.h"
#include <Flash.h>

#ifdef LOGGING
FLASH_STRING(HAVE_XBEE_PACKET_FROM_NODE,"Xbee from ");
FLASH_STRING(XBEE_SAMPLES_FOR_NODE,"Xbee Samples for ");
FLASH_STRING(PROG_PIN," pin ");
FLASH_STRING(SMOOTHED_OUT_TO," smoothed to ");
FLASH_STRING(NO_XBEE_PACKET,"No xbee\n");
FLASH_STRING(PROG_SAMPLE,"Sample ");
FLASH_STRING(PROG_EQUALS," = ");
FLASH_STRING(SUBMITTING_SAMPLES_TO,"Submitting to http://");
FLASH_STRING(HTTP_SUBMIT_CONNECTION_FAILED,"HTTP Connection Failed\n");
FLASH_STRING(HTTP_SUBMIT_REQUEST_FAILED,"HTTP Request Failed with: ");
#endif
//FLASH_STRING(CONTENT_LENGTH,"Content-Length: ");
//FLASH_STRING(SAMPLE_CONTENT_TYPE,"Content-Type: application/x-www-form-urlencoded");
#define CONTENT_LENGTH "Content-Length"
#define SAMPLE_CONTENT_TYPE "Content-Type: application/x-www-form-urlencoded"

unsigned long int lastSampleSubmission = 0;
byte samplesBeforeSubmit = NUM_SAMPLES * 2;

int analogSamples[NUM_ANALOG_PINS][NUM_SAMPLES];
char inputSamples[NUM_INPUT_PINS][NUM_SAMPLES];
int xbeeSamples[MAX_XBEE_NODES][NUM_XBEE_PINS];

void SetupSampler() {
  for (int i = 0; i < NUM_ANALOG_PINS; ++i) {
    for (int j = 0; j < NUM_SAMPLES; ++j) {
      analogSamples[i][j] = -1;
    }
  }
  for (int i = 0; i < NUM_INPUT_PINS; ++i) {
    for (int j = 0; j < NUM_SAMPLES; ++j) {
      inputSamples[i][j] = -1;
    }
  }
  for (int i = 0; i < MAX_XBEE_NODES; ++i) {
    for (int j = 0; j < NUM_XBEE_PINS; ++j) {
      xbeeSamples[i][j] = -1;
    }
  }
}

void TakeSamples() {
  boolean loggedSample = false;
  for (int i = 0; i < NUM_ANALOG_PINS; ++i) {
    loggedSample = false;
    if (analogPins[i] == -1) {
      continue;
    }
    for (int j = 0; j < NUM_SAMPLES; ++j) {
      if ((j == LAST_SAMPLE || analogSamples[i][j] == -1) && ! loggedSample) {
        analogSamples[i][j] = analogRead(analogPins[i]);
        loggedSample = true;
      } else if (analogSamples[i][LAST_SAMPLE] != -1 && j < LAST_SAMPLE) {
        // Shift samples down the array
        analogSamples[i][j] = analogSamples[i][j+1];
      }
    }
  }
  for (int i = 0; i < NUM_INPUT_PINS; ++i) {
    loggedSample = false;
    for (int j = 0; j < NUM_SAMPLES; ++j) {
      if ((j == LAST_SAMPLE || inputSamples[i][j] == -1) && ! loggedSample) {
        inputSamples[i][j] = digitalRead(analogPins[i]);
        loggedSample = true;
      } else if (inputSamples[i][LAST_SAMPLE] != -1 && j < LAST_SAMPLE) {
        // Shift samples down the array
        inputSamples[i][j] = inputSamples[i][j+1];
      }
    }
  }
  if (!xbeeSerial) {
    delay(5000);
  } else if (xbee.readPacket(5000)) {
    XBeeResponse response = xbee.getResponse();
    if (response.getApiId() == RX_16_IO_RESPONSE) {
      Rx16IoSampleResponse ioSample;
      xbee.getResponse().getRx16IoSampleResponse(ioSample);

      byte xbeeNode = ioSample.getRemoteAddress16();
      #ifdef LOGGING
      HAVE_XBEE_PACKET_FROM_NODE.print(Serial);
      Serial.println(xbeeNode,DEC);
      #endif
      if (xbeeNode < MAX_XBEE_NODES) {
        for (int i = 0; i < NUM_XBEE_PINS; ++i ) {
          int sampleSize = ioSample.getSampleSize();
          if (ioSample.isAnalogEnabled(i)) {
            int samples[sampleSize];
            for (int k = 0; k < sampleSize; ++k) {
              samples[k] = ioSample.getAnalog(i, k);
            }
            xbeeSamples[xbeeNode][i] = smoothSamples(samples,sampleSize);
            #ifdef LOGGING
            XBEE_SAMPLES_FOR_NODE.print(Serial);
            Serial.print(xbeeNode,DEC);
            PROG_PIN.print(Serial);
            Serial.print(i);
            SMOOTHED_OUT_TO.print(Serial);
            Serial.println(xbeeSamples[xbeeNode][i]);
            #endif
          } else if (ioSample.isDigitalEnabled(i)) {
            if (xbeeSamples[xbeeNode][i] == -1)
              xbeeSamples[xbeeNode][i] = 0;
            for (int k = 0; k < sampleSize; ++k) {
              xbeeSamples[xbeeNode][i] += ioSample.getAnalog(i, k) ? 1 : 0;
            }
          }
        }
      }
    }
  } else {
    #ifdef LOGGING
    NO_XBEE_PACKET.print(Serial);
    #endif
  }
}

void CheckAndSubmitSamples() {
  if (samplesBeforeSubmit > 0) {
    --samplesBeforeSubmit;
    return;
  }
  if ((now() - lastSampleSubmission) < SAMPLE_SUBMIT_FREQUENCY) {
    return;
  }
  
  int submitSamples[NUM_ANALOG_PINS];
  
  int contentLength = 0;
  for (int i = 0; i < NUM_ANALOG_PINS; ++i) {
    submitSamples[i] = -1;
    if (analogPins[i] == -1) {
      continue;
    }
    submitSamples[i] = smoothSamples(analogSamples[i], NUM_SAMPLES);
    #ifdef LOGGING
    PROG_SAMPLE.print(Serial);
    Serial.print(i);
    PROG_EQUALS.print(Serial);
    Serial.println(submitSamples[i]);
    #endif
    if (submitSamples[i] != -1) {
      contentLength += 7; // A0=FFF& or A10=FFF&
      if (analogPins[i] >= 10 )
        ++contentLength;
    }
  }
  
  for (int i = 0; i < MAX_XBEE_NODES; ++i) {
      for (int j = 0; j < NUM_XBEE_PINS; ++j) {
        if (xbeeSamples[i][j] != -1) {
          // X0P1=FFF&
          contentLength += 9;
        }
      }
  }

  
  EthernetClient c;
  HttpClient http(c);

  char configPath[22] = "/api/s/";
  const char hexDigits[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  const unsigned long int divisors [4] = { 0xFFFFFF, 0xFFFF, 0xFF, 0x01 };
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
  configPath[21] = '\0';

  #ifdef LOGGING
  SUBMITTING_SAMPLES_TO.print(Serial);
  Serial.print(YAHMS_SERVER);
  Serial.println(configPath);
  #endif
  http.beginRequest();
  if (http.post(YAHMS_SERVER, 80, configPath, USERAGENT) == 0)
  {
    // Request sent okay
//    #ifdef LOGGING
//    CONTENT_LENGTH.print(Serial);
//    Serial.println(contentLength);
//    #endif
//    CONTENT_LENGTH.print(http);
//    http.println(contentLength);
//    SAMPLE_CONTENT_TYPE.print(http);
//    http.println();
    http.sendHeader(CONTENT_LENGTH,contentLength);
    http.sendHeader(SAMPLE_CONTENT_TYPE);
    http.endRequest();
    
    for (int i = 0; i < NUM_ANALOG_PINS; ++i) {
      if (submitSamples[i] != -1) {
        http.print("A");
        http.print(analogPins[i],DEC);
        http.print("=");
        http.print(hexDigits[submitSamples[i]>>8 & 0x0F]);
        http.print(hexDigits[submitSamples[i]>>4 & 0x0F]);
        http.print(hexDigits[submitSamples[i] & 0x0F]);
        http.print("&");
      }
    }
    
    for (int i = 0; i < MAX_XBEE_NODES; ++i) {
      for (int j = 0; j < NUM_XBEE_PINS; ++j) {
        if (xbeeSamples[i][j] != -1) {
          // X0P1=FFF&
          http.print("X");
          http.print(i);
          http.print("P");
          http.print(j);
          http.print("=");
          http.print(hexDigits[xbeeSamples[i][j]>>8 & 0x0F]);
          http.print(hexDigits[xbeeSamples[i][j]>>4 & 0x0F]);
          http.print(hexDigits[xbeeSamples[i][j] & 0x0F]);
          http.print("&");
          xbeeSamples[i][j] = -1;
        }
      }
    }

    int err = http.responseStatusCode();
    if ((err >= 200) && (err < 300))
    {
      // We've got a successful response
      // And we're not interested in any of the headers
      err = http.skipResponseHeaders();
      if (err >= 0)
      {
      }
    } else {
      #ifdef LOGGING
      HTTP_SUBMIT_REQUEST_FAILED.print(Serial);
      Serial.println(err);
      #endif

    }
    http.stop();
    c.stop();
  } else {
    #ifdef LOGGING
    HTTP_SUBMIT_CONNECTION_FAILED.print(Serial);
    #endif
  }

  // Have to assume it worked really
  lastSampleSubmission = now();
  
}

int smoothSamples( int samples[], int numSamples) {
    
  int diff = 0, total = 0, average = 0, number = 0, deviation = 0;
  
  for (int j = 0; j < numSamples; ++j) {
    if (samples[j] != -1) {
      total += samples[j];
      ++number;
    }
  }
  if (number > 0) {
    average = total / number;
    for (int j = 0; j < numSamples; ++j) {
      if (samples[j] != -1) {
        deviation += abs(samples[j] - average);
      } 
    }
    deviation = deviation / number;
    number = 0;
    total = 0;
    for (int j = 0; j < numSamples; ++j) {
      if (samples[j] != -1) {
        diff = abs(samples[j] - average);
        if ( deviation == 0 || diff <= deviation) {
          total += samples[j];
          ++number;
        }
      } 
    }
    average = total / number;
    return average;
  } else {
    return -1;
  }
}
