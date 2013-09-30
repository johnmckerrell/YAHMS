# YAHMS

* Yet Another Home Management System

* John McKerrell 2011/2012

YAHMS is distributed under the terms of the Artistic License 2.0. For more
details, see the full text of the license in the file LICENSE.

YAHMS acts as a simple Arduino sketch that takes its instructions from
a server on the internet. The only thing that requires changing is the MAC
address of the Arduino Ethernet board or Ethernet Shield which should be
put in `YAHMS_Local.h`. Configuration of input & output pins is done on the
server.

The server will define which digital output and analog input pins are
active. If an XBee module is attached the server should tell the Arduino
which pins this is connected to.

The server will store "control blocks" which the YAHMS code downloads and
uses to determine when to turn the digital input pins on and off. This is
time based, the YAHMS code uses NTP to ensure accurate timings and a
timezone offset which is sent by the server.

When analog inputs are active the YAHMS sketch will sample these and send
a normalised value back to the server once every minute.

Similarly if an XBee module is active, values reported by XBee packets
will be normalised and sent back to the server too.

The server can be found at http://yahms.net/ but is very minimal,
ask John McKerrell at http://twitter.com/mcknut if you want to use it.

More info can be found here:

http://blog.johnmckerrell.com/category/arduino-2/

Though this code is quite functional it should still be considered an
early release. Unfortunately the latest releases require an extra feature
in the Ethernet libraries which has not yet been released. Again, contact
John McKerrell for more info. It requires the latest Arduino interface and
the following libraries:

XBee with SoftwareSerial support
https://github.com/johnmckerrell/xbee-arduino

Flash
http://arduiniana.org/libraries/flash/

Time
http://arduino.cc/playground/Code/Time

HttpClient
https://github.com/amcewen/HttpClient
