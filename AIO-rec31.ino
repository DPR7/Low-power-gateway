// 
// Tiny MQTT Gateway by @TOM from info at aquapoeder.nl
//
// https://creativecommons.org/licenses/by-nc-sa/2.0/
// Feel free to send me your adaptions..
//
/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "XXX"
#define WLAN_PASS       "YYY"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "ZZZ"
#define AIO_KEY         "RRR"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// RF val setup
float data1 = 0;
float data2 = 0;
float data3 = 0;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.

Adafruit_MQTT_Publish Data1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/d1");
Adafruit_MQTT_Publish Data2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/d2");
Adafruit_MQTT_Publish Data3 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/d3");


// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);

  mySwitch.enableReceive(14);  // Receiver on interrupt 0 => that is pin #2

  
  Serial.println(F("MQTT Gateway"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton);
}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

if (mySwitch.available()) {
    
    int value = mySwitch.getReceivedValue();
    
    if (value == 0) {
      Serial.print("Unknown encoding");
    } else {
      Serial.print("Received ");
      Serial.println( mySwitch.getReceivedValue() );

   // Decoder for HUM
      if ((value > 31000) && (value < 32000))
        { 
        data1 = (value - 31000); 
        data1 = (data1/10);
        Serial.println( data1 );
        delay (240);
        } 

    // Decoder for TEMP
      if ((value > 33000) && (value < 34000))
        { 
        data2 = (value - 33000); 
        data2 = (data2/10);
        Serial.println( data2 );
        delay (240);
        }

    // Decoder for HYGRO
      if ((value > 34000) && (value < 35000))
        { 
        data3 = (value - 34000); 
        Serial.println( data3 );
        delay (240);
        } 

  // End of else loop     
  }
  
//      Serial.print( mySwitch.getReceivedBitlength() );
//      Serial.print("bit ");
//      Serial.print("Protocol: ");
//      Serial.println( mySwitch.getReceivedProtocol() );
  

  mySwitch.resetAvailable();
  }
    
  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
  if (subscription == &onoffbutton) {
//  Serial.print(F("Got: "));
//  Serial.println((char *)onoffbutton.lastread);
   }
   }

  // Now we can publish stuff!
  Serial.print(F("\nSending data1 val "));
  Serial.print(data1);
  Serial.print("...");
  if (! Data1.publish(data1)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // Now we can publish stuff!
  Serial.print(F("\nSending data2 val "));
  Serial.print(data2);
  Serial.print("...");
  Data2.publish(data2);

  Serial.print(F("\nSending data2 val "));
  Serial.print(data3);
  Serial.print("...");
  Data3.publish(data3);

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
    }

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
