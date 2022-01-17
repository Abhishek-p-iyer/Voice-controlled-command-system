
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define SCROLL_DELAY 75


char* str;
String payload;
uint32_t present;
bool first_time;
uint16_t  scrollDelay;  // in milliseconds
#define  CHAR_SPACING  1 // pixels between characters


#define BUF_SIZE  75
char curMessage[BUF_SIZE];
char newMessage[BUF_SIZE];
bool newMessageAvailable = false;

ESP8266WiFiMulti WiFiMulti;

#define  MAX_DEVICES 8
//#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW --- corrected code without error 


#define  CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS


#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "AbhishekIyer"
#define AIO_KEY         "aio_qJpF524OlclpLCN2w4uveYF8FHlt"



WiFiClient client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe message = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/ece3501-iot-jcomp");

void MQTT_connect();
//MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES); ----code without error 
MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);
// Arbitrary pins
//MD_MAX72XX mx = MD_MAX72XX(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);





uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t)
{
  static char   *p = curMessage;
  static uint8_t  state = 0;
  static uint8_t  curLen, showLen;
  static uint8_t  cBuf[8];
  uint8_t colData;

  switch (state)
  {
    case 0: // Load the next character from the font table
      showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state++;

      if (*p == '\0')
      {
        p = curMessage;     // reset the pointer to start of message
        if (newMessageAvailable)  // there is a new message waiting
        {
          strcpy(curMessage, str);  // copy it in
          newMessageAvailable = false;
        }
      }

    case 1: // display the next part of the character
      colData = cBuf[curLen++];
      if (curLen == showLen)
      {
        showLen = CHAR_SPACING;
        curLen = 0;
        state = 2;
      }
      break;

    case 2: // display inter-character spacing (blank column)
      colData = 0;
      curLen++;
      if (curLen == showLen)
        state = 0;
      break;

    default:
      state = 0;
  }

  return (colData);
}

void scrollText(void)
{
  static uint32_t prevTime = 0;

  if (millis() - prevTime >= scrollDelay)
  {
    mx.transform(MD_MAX72XX::TSL);
    prevTime = millis();   
  }
}
void  no_connection(void)
{
  newMessageAvailable = 1;
  strcpy(curMessage, "No Internet! ");
  scrollText();


}
void setup()
{
  mx.begin();
  mx.setShiftDataInCallback(scrollDataSource);

  scrollDelay = SCROLL_DELAY;
  strcpy(curMessage, "Hello! ");
  newMessage[0] = '\0';

  Serial.begin(57600);
  Serial.print("\n[MD_MAX72XX Message Display]\nType a message for the scrolling display\nEnd message line with a newline");

  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("Zephyrus_Guest", "12345678");
  Serial.println("Connecting");
  newMessageAvailable = 1;
  present = millis();
  first_time = 1;
  mqtt.subscribe(&message);
 str = "**IOT PROJECT*";
}

void loop()
{
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    delay(1000);
  }
  MQTT_connect();
  
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1))) {
    if (subscription == &message) {
      payload ="";
      Serial.print(F("Got: "));
      Serial.println((char *)message.lastread);     
      str = (char*)message.lastread;
      payload = (String) str;
      //payload.toUpperCase();        
      payload += "       ";
      str = &payload[0];      
      newMessageAvailable = 1;
    }
  }

  scrollText();
}
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { 
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  
       retries--;
       if (retries == 0) {
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
