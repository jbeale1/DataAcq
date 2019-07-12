// Simple EtherShield ENC28J60 webserver for Arduino (JeeNode)
// J.Beale 12-July-2019

#include <EtherCard.h>

static byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
static byte dnsip[] = { 192, 168, 1, 1 };
static byte gwip[] = { 192, 168, 1, 1 };
static byte myip[] = { 192, 168, 1, 71 };

const char rhost[] PROGMEM = "www.thingspeak.com";  // just for example

byte Ethernet::buffer[700];  // TCP/IP send & receive buffer
BufferFiller bfill;

typedef struct {    // data structure to be sent
 int sec;
 int power;
 int temp;
} Payload;

Payload measurement;   // stored data to transmit

unsigned long lastRead;    // millisecond timestamp of last analog input reading
unsigned int rcount = 0;            // count how many HTML responses issued
unsigned int msTimeout = 1000;      // how many milliseconds between analog input readings

// ----------------------------------------------------

const char HttpNotFound[] PROGMEM =
  "HTTP/1.0 404 Unauthorized\r\n"
  "Content-Type: text/html\r\n\r\n"
  "<h1>404 Page Not Found</h1>";


const char okHeader[] PROGMEM = 
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    ;

// ======================================================================
void setup ()
{
  Serial.begin(57600);
  Serial.println("\n[Jeenode Simple WWW Server v0.2 12-July-2019]\n");
  // Jeenode Ethercard uses Pin 8 as SS line
  if (ether.begin(sizeof Ethernet::buffer, mymac, 8) == 0)
  {
    Serial.println("Failed to access Ethernet controller");
  }
  
  // ether.staticSetup(myip, gwip, dnsip);  // for declaring a static IP
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");
  ether.printIp("My IP: ", ether.myip);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
  Serial.println("-----\n");

  // if (!ether.dnsLookup(rhost)) Serial.println("DNS failed");
  // ether.printIp("Server: ", ether.hisip);

  pinMode(0,INPUT);        // use pin 0 as (analog) input
  lastRead = millis() + msTimeout + 1;  // start with a timeout condition
}

static word dashboard1() {
  rcount++;  // this counts as one more HTML reply
  bfill = ether.tcpOffset();
  bfill.emit_p(PSTR(
    "$F\r\n"
    "<html><head>"
    "<link rel=\"icon\" href=\"data:,\">"
    "</head>"
    "<body>Analog Data<br>"
    "Count: $D Time:$D Power:$D Temp:$D</body></html>"),
     okHeader, rcount, measurement.sec, measurement.power, measurement.temp);
  // Serial.println(measurement.sec);  // DEBUG  what time is it?
  return bfill.position();
}

// ===================================================================
void loop ()
{
  word len = ether.packetReceive();  // len = how many bytes in rec packet
  word pos = ether.packetLoop(len);
  if (pos)  // check if valid tcp data is received
  {
    bfill = ether.tcpOffset();
    char* data = (char*) Ethernet::buffer + pos;
    
    data[len] = 0x00;  // null-terminate buffer to make it a string
    Serial.println(data); // display entire string
    Serial.print(len); // how many characters in received packet
    Serial.println(" ----"); // separator
    
    if (strncmp("GET /", data, 5) != 0)
    {
      bfill.emit_p(HttpNotFound);  // no idea in what case this happens
    }
    else
    {
      data += 5;  // increment buffer to skip over "GET /"
      if (strncmp("?stuff", data, 6) == 0)  {
        dashboard1();
        Serial.print("asked for stuff\n");        
      } else  {
        dashboard1();
      }
    }
    ether.httpServerReply(bfill.position());    // send http response

    // DEBUG report data on local serial port
    Serial.print(rcount); // how many total replies
    Serial.print(", ");
    Serial.print(measurement.sec); 
    Serial.print(", ");
    Serial.print(measurement.power);
    Serial.print(", ");
    Serial.print(measurement.temp); 
    Serial.println("  =======\n"); // separator
       
  }

  // update recorded data every msTimeout interval
  if ((millis() - lastRead) > msTimeout) {
         lastRead = millis();
         measurement.sec = (int) (lastRead / 1000);
         measurement.power = analogRead(0); 
         measurement.temp = analogRead(1);
       }
} // end loop()
