// This #include statement was automatically added by the Spark IDE.
#include "idDHT22RH/idDHT22RH.h"

#include "application.h"

/****************************************************************************
*   Project: Weather Station NG                                           *
*                                                                           *
****************************************************************************/
extern char* itoa(int a, char* buffer, unsigned char radix);

#define version 107



// Time (seconds) for each sleep
#define PERIOD 300
// Set UPDATE2CLOUD to true if you want the measurement sent to the cloud
#define UPDATE2CLOUD true 
// Thingspeak API WRITE key
#define THINGSPEAK_API_WRITEKEY "THINGSPEAK_API_WRITEKEY"

#define BACKLDUR 10000 // 10s
#define AMACQTIMEOUT 1000
#define NETTIMEOUT 60000
#define CHTUPDPERIOD 300 // update every 300s

int lastT;
int trigger = 0;
bool refresh = true;
bool readOK = false;
bool flip = false;
int AMFailCount = 0;

// Pin out defs
int led = D7;
int RST = A4;
int AMPower = D3;


// declaration for AM2321 handler
int idDHT22pin = D4; //Digital pin for comunications
void dht22_wrapper(); // must be declared before the lib initialization
String status;
float temp, humi = 0;
// AM2321 instantiate
idDHT22 DHT22(idDHT22pin, dht22_wrapper);


// Thingspeak.com API
TCPClient client;
const char * WRITEKEY = THINGSPEAK_API_WRITEKEY;
const char * serverName = "api.thingspeak.com";
IPAddress server = {184,106,153,149};
int sent = 0;

// Indicator color
byte r=0, g=0, b=0;
unsigned long rgb = 0;


/****************************************************************************
*****************************************************************************
****************************  Data Upload   *********************************
*****************************************************************************
****************************************************************************/

void sendToThingSpeak(const char * key, String mesg)
{
    int tries=5;
    
    sent = 0;
    client.stop();
    int lt = millis();
    RGB.control(true);
    RGB.color(255,255,0);
    while (!client.connect(server,80) && (tries-- > 0)) {
        RGB.color(80,80,0);
        delay(500);
        RGB.color(255,255,0);
    }
    if (tries > 0) {
        RGB.color(128,0,255);
        client.print("POST /update");
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(serverName);
        client.println("User-Agent: Spark");
        client.println("Connection: close");
        client.print("X-THINGSPEAKAPIKEY: ");
        client.println(key);
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-length: ");
        client.println(mesg.length());
        client.println();
        client.println(mesg);
        client.flush();
        lt = millis();
        while (client.available() == false && millis() - lt < 1000) {
            // Just wait up to 1000 millis
            delay(200);
        }
        lt = millis();
        while (client.available() > 0 && millis() - lt < NETTIMEOUT) {
            client.read();
        }
        client.flush();
        client.stop();
        sent = 1;
        RGB.color(0,255,0);
    } else{
        sent = 0;
        RGB.color(255,0,0);
    }
    delay(5000);
    RGB.control(false);
    return;
}



int myVersion(String command)
{
    return version;
}

void publishReadings()
{
    int res = 1;
    int tries = 5;

    res = readAM2321();
    while (res && (tries-- > 0)) {
        delay(3000);
        res = readAM2321();
    }
    if (!res) {
        char szEventInfo[64];
        sprintf(szEventInfo, "field1=%.1f&field2=%.2f", temp, humi);
        sendToThingSpeak(WRITEKEY,String(szEventInfo));
    }
    Spark.sleep(SLEEP_MODE_DEEP, CHTUPDPERIOD);
    return;
}


// This wrapper is in charge of calling
// mus be defined like this for the lib work
void dht22_wrapper() {
  DHT22.isrCallback();
  return;
}

int readAM2321() {
  int lt = millis();
  int result;

  DHT22.acquire();
  while (DHT22.acquiring() && (millis() - lt < AMACQTIMEOUT))
    ;
  if (millis() - lt >= AMACQTIMEOUT) {
      result = IDDHTLIB_ERROR_ACQUIRING;
  } else {
      result = DHT22.getStatus();
  }
  switch (result)
  {
    case IDDHTLIB_OK:
      status = "OK                 ";
      break;
    case IDDHTLIB_ERROR_CHECKSUM:
      status = "Checksum           ";
      break;
    case IDDHTLIB_ERROR_ISR_TIMEOUT:
      status = "ISR Time out       ";
      break;
    case IDDHTLIB_ERROR_RESPONSE_TIMEOUT:
      status = "Response time out  ";
      break;
    case IDDHTLIB_ERROR_DATA_TIMEOUT:
      status = "Data time out      ";
      break;
    case IDDHTLIB_ERROR_ACQUIRING:
      status = "Acquiring          ";
      break;
    case IDDHTLIB_ERROR_DELTA:
      status = "Delta time to small";
      break;
    case IDDHTLIB_ERROR_NOTSTARTED:
      status = "Not started        ";
      break;
    default:
      status = "Unknown            ";
      break;
  }
  if (!result) {
    temp = DHT22.getCelsius();
    humi = DHT22.getHumidity();
  }
  return result;
}

void setRGBColor(float temperature) {
    int i;
    float h, f;
    int q, t;
    
    h = 300 - (temperature + 10) * 300 / 50;
    h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	q = (int)((1 - f)*255);
	t = (int)(f * 255);

switch( i ) {
		case 0:
			r = 255;
			g = t;
			b = 0;
			break;
		case 1:
			r = q;
			g = 255;
			b = 0;
			break;
		case 2:
			r = 0;
			g = 255;
			b = t;
			break;
		case 3:
			r = 0;
			g = q;
			b = 255;
			break;
		case 4:
			r = t;
			g = 0;
			b = 255;
			break;
		default:		// case 5:
			r = 255;
			g = 0;
			b = q;
			break;
	}
	rgb = r<<16;
	rgb |= g<<8;
	rgb |= b;
	return;
}

/****************************************************************************
*****************************************************************************
**************************  Initialization  *********************************
*****************************************************************************
****************************************************************************/

void setup()
{
    Time.zone(8);
    pinMode(led, OUTPUT);
    pinMode(AMPower, OUTPUT);
    digitalWrite(led, HIGH);
    // Turn on AM2322
    digitalWrite(AMPower, HIGH);
    delay(3000);
    AMFailCount=0;
}

/****************************************************************************
*****************************************************************************
**************************  Main Proc Loop  *********************************
*****************************************************************************
****************************************************************************/

void loop()
{
    digitalWrite(led, HIGH);
    if (!readAM2321()) {
        digitalWrite(led, LOW);
        delay(2500);
        publishReadings();
    } else {
        // Read from AM2322 failed, likely permanant, poweroff and reset upon 3
        AMFailCount++;
        digitalWrite(led, LOW);
        if (AMFailCount > 3) {
            digitalWrite(AMPower, LOW); // Power off AM2322
            delay(1500);
            pinMode(RST, OUTPUT);
            digitalWrite(RST, LOW); // Reset Core
        } else {
            delay(500);
        }
    }
}
