/*

 Udp NTP Client
 
 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket 
 For more on NTP time servers and the messages needed to communicate with them, 
 see http://en.wikipedia.org/wiki/Network_Time_Protocol
 
 created 4 Sep 2010 
 by Michael Margolis
 modified 17 Sep 2010
 by Tom Igoe
 
 This code is in the public domain.

 */

#include <SPI.h>         
#include <Ethernet.h>
#include <Udp.h>
#include <Time.h>

void SyncTime_setup();
unsigned long getNtpTime();
unsigned long sendNTPpacket(IPAddress& address);
