// ----------------------------------------------
// Display charging status via RGB LED
// MQTT  based
// 23.02.2021
// ----------------------------------------------

#include <MQTT.h>
#include "EEPROM.h"

// -------------------------------------- ESP32 Wifi Hotspot Configuration -----------------------------

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>

// ---------------------------------------------- ESP32 Wifi Hotspot Configuration -----------------------------------

const char ssid[] = "";
const char pass[] = "";

const byte led_gpio_b = 32;   // blue
const byte led_gpio_g = 25;   // green
const byte led_gpio_r = 33;   // red

uint32_t chipId = 0;

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;
int addr = 0;

#define EEPROM_SIZE 21
char uuid[EEPROM_SIZE] = "POWERPLANT0000000000";
String myUUID = "";

// ---------------------------------------------------------- connect -----------------------------------------------

void connect() {

  Serial.println("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("connecting...");
  String clientId = "Datacar-" + myUUID;
  digitalWrite(led_gpio_r, LOW);   // turn the LED on (HIGH is the voltage level)


  while (!client.connect(clientId.c_str(), "broker", "password")) {

    digitalWrite(led_gpio_r, HIGH);   // turn the LED on (HIGH is the voltage level)
    Serial.print(".");
    delay(1000);
    digitalWrite(led_gpio_r, LOW);
    delay(1000);

  }

  digitalWrite(led_gpio_r, LOW);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(led_gpio_g, HIGH);
  digitalWrite(led_gpio_b, LOW);

  Serial.println("connected. Subscribe to /powerplant/" + myUUID);
  client.subscribe("/powerplant/" + myUUID);
  client.publish("/powerplant/" + myUUID, myUUID + " has registered.");

  digitalWrite(led_gpio_r, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(led_gpio_g, HIGH);
  digitalWrite(led_gpio_b, HIGH);
  // client.unsubscribe("/hello");
}

// ---------------------------------------------- MQTT messageReceived -----------------------------------


void messageReceived(String &topic, String &payload) {

  Serial.println("incoming: " + topic + " - " + payload + " Length: " + payload.length() );
  // the paylod contains the RGB pattern
  if (payload.length() == 6) {

    if (payload.startsWith("RGB", 0)) {

      // red
      if (payload.substring(3, 4) == "1") {
        // Serial.print("Enable red.");
        digitalWrite(led_gpio_r, HIGH);   // turn the LED on (HIGH is the voltage level)
      } else {
        digitalWrite(led_gpio_r, LOW);
      }

      // green
      if (payload.substring(4, 5) == "1") {
        // Serial.print("Enable green.");
        digitalWrite(led_gpio_g, HIGH);   // turn the LED on (HIGH is the voltage level)
      } else {
        digitalWrite(led_gpio_g, LOW);
      }

      // blue
      if (payload.substring(5, 6) == "1") {
        // Serial.print("Enable blue.");
        digitalWrite(led_gpio_b, HIGH);   // turn the LED on (HIGH is the voltage level)
      } else {
        digitalWrite(led_gpio_b, LOW);
      }

    }
  }

}

// ---------------------------------------------- setup -----------------------------------

void setup() {

  Serial.begin(115200);
  // Serial.print("PowerPlant entering Setup.");

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(led_gpio_g, OUTPUT);
  pinMode(led_gpio_b, OUTPUT);
  pinMode(led_gpio_r, OUTPUT);

  // Pink, waiting for WiFi.
  digitalWrite(led_gpio_r, HIGH);
  digitalWrite(led_gpio_b, HIGH);

  // ------------------------ INIT UUID ----------------------------

  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("failed to init EEPROM");
  }

  // reading byte-by-byte from EEPROM
  for (int i = 0; i < EEPROM_SIZE; i++) {
    byte readValue = EEPROM.read(i);

    if (readValue == 0) {
      break;
    }

    char readValueChar = char(readValue);
    myUUID = myUUID + String(readValueChar);
  }

  if (myUUID.startsWith("POWERPLANT", 0)) {

    myUUID.toCharArray(uuid, 64);
    Serial.println("Got UUID from flash memory: " + myUUID);

  } else {

    // set default
    myUUID = "POWERPLANT0000000000";
    myUUID.toCharArray(uuid, 64);

    // randomize
    for (int i = 10; i < 20; i++) {
      uuid[i] = 0x41 | random(0, 25);
    }

    // and set
    myUUID = String(uuid);

    // writing byte-by-byte to EEPROM
    for (int i = 0; i < EEPROM_SIZE; i++) {
      EEPROM.write(addr, uuid[i]);
      addr += 1;
    }

    EEPROM.commit();
    Serial.println("Created uuid and store in Flash memory: " + myUUID);

  }

  Serial.println("----------------------------------------------------");
  Serial.println("Device ID: " + myUUID.substring(10, 20));
  Serial.println("----------------------------------------------------");

  // ------------------------ INIT WiFi ----------------------------

  WiFiManager wifiManager;
  wifiManager.autoConnect("PowerPlant");

  // ------------------------ INIT MQTT ----------------------------

  digitalWrite(led_gpio_r, HIGH);
  client.begin("BROKER_URL", net);
  client.onMessage(messageReceived);
  digitalWrite(led_gpio_r, HIGH);
  connect();

}

// ---------------------------------------------- loop -----------------------------------

void loop() {

  client.loop();
  delay(100);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

}


// ---------------------------------------------- end -----------------------------------