// ----------------------------------------------
// Display charging status via RGB LED (NeoPixel)
// Last Update 15.03.2021
//
// Sebastian Boettger - seb at sono dot news
// ----------------------------------------------

#include <MQTT.h>
#include <EEPROM.h>
#include <WiFiConnect.h>
#include <WiFiClient.h>

#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266HTTPClient.h>
#else
#include <HTTPClient.h>
#endif

// ---------------------------------------------- ESP32 Wifi Hotspot Configuration -----------------------------------

const char ssid[] = "";
const char pass[] = "";

int blink_mode = 0;           // 0 = off; 1 = slow; 2 = fast;
int patternUpdated = 0;

uint32_t chipId = 0;
int brightness = 64;

int temp_monitor_count = 55555;

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define NEOPIXEL_PIN 33 // 19 beim ersten Board (33 orig)
#define NUMPIXELS 7

#ifdef __cplusplus
  extern "C" {
 #endif

  uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500
  
WiFiConnect wc;

HTTPClient http;
WiFiClient wifi_client;
MQTTClient mqtt_client;

unsigned long lastMillis = 0;
int addr = 0;

#define EEPROM_SIZE 21
char uuid[EEPROM_SIZE] = "POWERPLANT0000000000";
String myUUID = "";

int opMode = 0;
String opParameter = "";

// ---------------------------------------------------------- connect -----------------------------------------------

void connectToMQTT() {

  Serial.println("connecting...");
  String clientId = "Datacar-" + myUUID;
  
  int counter = 0;
  pixels.clear();
  pixels.setBrightness(brightness);
  
  while (!mqtt_client.connect(clientId.c_str())) {     

    pixels.setPixelColor(counter, pixels.Color(0, 64, 64, 64));
    pixels.show();
    Serial.print(".");
    delay(1000);

    counter++;
    if (counter == NUMPIXELS) {
      counter = 0;
      pixels.clear();
      Serial.print("No MQTT connection. Retrying.\n");
    }

  }

  Serial.println("connected. Subscribe to /sion_status/powerplant/" + myUUID);
  mqtt_client.subscribe("/sion_status/powerplant/" + myUUID);
  mqtt_client.publish("/sion_status/powerplant/" + myUUID, "Device " + myUUID + " registered.");
 
}

// ---------------------------------------------- MQTT messageReceived -----------------------------------

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void messageReceived(String &topic, String &payload) {

  Serial.println("incoming: " + topic + " - " + payload + " Length: " + payload.length() );

    if ( payload.length() > 0) {


      int lastOpMode = opMode;
      
      if (payload.indexOf(":") > 0) {
  
            // if pixel pattern  
            String prefix = getValue(payload, ':', 0);
            String suffix = getValue(payload, ':', 1);

            // Serial.print("X:" + prefix);  
            // Serial.println("Y:" + suffix);
            opMode = prefix.toInt();
            opParameter = suffix;
            Serial.println("Display Mode: " +  String(opMode) + " (" + opParameter + ")");
            patternUpdated = 1;

        // special case - brightness
        if (opMode == 99) {
            brightness = opParameter.toInt();
            if (brightness < 1) { brightness = 64; }
            pixels.setBrightness(brightness);
            Serial.println("Set brightness to: " + String(brightness));
            opMode = lastOpMode;
        }            

      } else {

        opMode = payload.toInt();
        Serial.println("Display Mode: " + String(opMode));

        patternUpdated = 1;
      }


        if ((opMode > 100) && (opMode < 200)) {
          blink_mode = 1;
        }
        
    } // payload

    

}

// ----------------------------------------------------- NEOPIXEL FANCY PATTERNS -----------------------------------------------
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.

uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


void rainbowCycle(uint8_t wait) {
  uint16_t r, j;
 
  for(j=0; j<256*6; j++) { // 6 cycles of all colors on wheel
    for(r=0; r< pixels.numPixels(); r++) {
      pixels.setPixelColor(r, Wheel(((r * 256 / pixels.numPixels()) + j) & 255));
    }
    pixels.show();
    delay(wait);
  }
}


// ----------------------------------------------------- SET PATTERN ACCORDING TO VALUE -----------------------------------------------


void updatePixelPattern() {

  blink_mode = 0;
    
  if (patternUpdated > 0) {
     
     pixels.clear();

     // ---- default mode, center bright white
     if (opMode == 0) {               
        pixels.setPixelColor(0, 255, 255, 255);        
     }
     
     // ---- single dot - white
     if (opMode == 1) {               
        pixels.setPixelColor(0, 255, 255, 255);                  
     }
     
     // ---- single dot - red
     if (opMode == 2) {               
        pixels.setPixelColor(0, 255, 0, 0);                  
     }

     // ---- single dot - green
     if (opMode == 3) {        
        pixels.setBrightness(brightness);
        pixels.setPixelColor(0, 0, 255, 0);                  
     }

     // ---- single dot - blue
     if (opMode == 4) {        
        pixels.setBrightness(brightness);
        pixels.setPixelColor(0, 0, 0, 255);                  
     }


     // ---- single dot - default yellow
     if (opMode == 5) {               
        pixels.setPixelColor(0, 255, 196, 0);                  
     }

     // ---- single dot - cyan
     if (opMode == 6) {        
        pixels.setBrightness(brightness);
        pixels.setPixelColor(0, 0, 255, 255);                  
     }

     // ---- single dot - magenta
     if (opMode == 7) {        
        pixels.setBrightness(brightness);
        pixels.setPixelColor(0, 255, 0, 255);                  
     }

     // smart white all colors
     if (opMode == 10) {                   
        pixels.setPixelColor(0, 255, 255, 255);      
        pixels.setPixelColor(1, 128, 128, 128);      
        pixels.setPixelColor(2, 128, 128, 128);  
        pixels.setPixelColor(3, 128, 128, 128);  
        pixels.setPixelColor(4, 128, 128, 128);  
        pixels.setPixelColor(5, 128, 128, 128);  
        pixels.setPixelColor(6, 128, 128, 128);          
     }

     
     // ---- all colors - white
     if (opMode == 11) {        
        uint32_t clr = pixels.Color(255, 255, 255);  
        pixels.fill(clr, 0, NUMPIXELS);          
     }

     // ---- all colors - red
     if (opMode == 12) {        
        uint32_t clr = pixels.Color(255, 0, 0);  
        pixels.fill(clr, 0, NUMPIXELS);          
     }

     // ---- all colors - green
     if (opMode == 13) {        
        uint32_t clr = pixels.Color(0, 255, 0);  
        pixels.fill(clr, 0, NUMPIXELS);            
     }

     // ---- all colors - blue
     if (opMode == 14) {        
        uint32_t clr = pixels.Color(0, 0, 255);  
        pixels.fill(clr, 0, NUMPIXELS);             
     }     


     // ---- all colors - default yellow
     if (opMode == 15) {        
        uint32_t clr = pixels.Color(255, 196, 0);  
        pixels.fill(clr, 0, NUMPIXELS);          
     }

     // ---- all colors - cyan
     if (opMode == 16) {        
        uint32_t clr = pixels.Color(0, 255, 255);  
        pixels.fill(clr, 0, 7);            
     }

     // ---- all colors - magenta
     if (opMode == 17) {        
        uint32_t clr = pixels.Color(255, 0, 255);  
        pixels.fill(clr, 0, NUMPIXELS);             
     }     
    
     // ---- custom color set in opPattern - center / single - "95:255188000"
     if (opMode == 95) {          
        int col1 = opParameter.substring(0,3).toInt();
        int col2 = opParameter.substring(3,6).toInt();
        int col3 = opParameter.substring(6,9).toInt();            
        pixels.setPixelColor(0, col1, col2, col3);                  
     }

     // ---- custom color set in opPattern - all / full - "96:255188000"
     if (opMode == 96) {          
        int col1 = opParameter.substring(0,3).toInt();
        int col2 = opParameter.substring(3,6).toInt();
        int col3 = opParameter.substring(6,9).toInt();      
        uint32_t clr = pixels.Color(col1, col2, col3);  
        pixels.fill(clr, 0, NUMPIXELS);                         
     }

     
     // ---- static rainbow - dark
     if (opMode == 97) {        
        pixels.setPixelColor(0, 0, 0, 0);      
        pixels.setPixelColor(1, 255, 0, 0);      
        pixels.setPixelColor(2, 255, 255, 0);  
        pixels.setPixelColor(3, 0, 255, 0);  
        pixels.setPixelColor(4, 0, 255, 255);  
        pixels.setPixelColor(5, 0, 0, 255);  
        pixels.setPixelColor(6, 255, 0, 255);  
     }


     // ---- static rainbow - light
     if (opMode == 98) {        
        pixels.setPixelColor(0, 255, 255, 255);      
        pixels.setPixelColor(1, 255, 0, 0);      
        pixels.setPixelColor(2, 255, 255, 0);  
        pixels.setPixelColor(3, 0, 255, 0);  
        pixels.setPixelColor(4, 0, 255, 255);  
        pixels.setPixelColor(5, 0, 0, 255);  
        pixels.setPixelColor(6, 255, 0, 255);  
     }


     // ---- set brightness
     if (opMode == 99) { 
        pixels.setBrightness(brightness);
     } 


     // ---- single dot - dark / black
     if (opMode == 100) {               
        pixels.setPixelColor(0, 0, 0, 0);                  
     }

     
    // ende static pattern
    pixels.show();
    
  } else {

     // dynamnic elements we always do

         // ---- single dot - white blinking
     if (opMode == 101) {               
        pixels.setPixelColor(0, 0, 0, 0);     
        pixels.show();       
        delay (600);           
        pixels.setPixelColor(0, 255, 255, 255);            
        pixels.show();
        delay (500);         
     }
     
     // ---- single dot - red blinking
     if (opMode == 102) {               
        pixels.setPixelColor(0, 0, 0, 0);     
        pixels.show();       
        delay (600);           
        pixels.setPixelColor(0, 255, 0, 0);            
        pixels.show();
        delay (500);         
     }

     // ---- single dot - green blinking
     if (opMode == 103) {               
        pixels.setPixelColor(0, 0, 0, 0);     
        pixels.show();       
        delay (600);           
        pixels.setPixelColor(0, 0, 255, 0);            
        pixels.show();
        delay (500);         
     }

     // ---- single dot - blue blinking
     if (opMode == 104) {               
        pixels.setPixelColor(0, 0, 0, 0);     
        pixels.show();       
        delay (600);           
        pixels.setPixelColor(0, 0, 0, 255);            
        pixels.show();
        delay (500);         
     }     

     // ---- single dot - blue blinking
     if (opMode == 105) {               
        pixels.setPixelColor(0, 0, 0, 0);     
        pixels.show();       
        delay (600);           
         pixels.setPixelColor(0, 255, 196, 0);       
        pixels.show();
        delay (500);         
     }    

     // ---- single dot - cyan blinking
     if (opMode == 106) {               
        pixels.setPixelColor(0, 0, 0, 0);     
        pixels.show();       
        delay (600);           
        pixels.setPixelColor(0, 0, 255, 255);            
        pixels.show();
        delay (500);         
     }         
 // ---- animated - red circled
     if (opMode == 112) {       
        pixels.clear();        
        for (int i = 1; i < (NUMPIXELS-1); i = i + 1) {
          pixels.clear();     
          pixels.setPixelColor(i, 255, 0, 0);     
          pixels.show();       
          delay (250);           
        }
        pixels.clear();  
        pixels.setPixelColor((NUMPIXELS-1), 255, 0, 0);     
        pixels.show();   
        delay (150);       
     }     

 // ---- animated - green circled
     if (opMode == 113) {       
        pixels.clear();        
        for (int i = 1; i < (NUMPIXELS-1); i = i + 1) {
          pixels.clear();     
          pixels.setPixelColor(i, 0, 255, 0);     
          pixels.show();       
          delay (250);           
        }
        pixels.clear();  
        pixels.setPixelColor((NUMPIXELS-1), 0, 255, 0);     
        pixels.show();   
        delay (150);       
     }     


    // ---- animated - blue circled
     if (opMode == 114) {       
        pixels.clear();        
        for (int i = 1; i < (NUMPIXELS-1); i = i + 1) {
          pixels.clear();     
          pixels.setPixelColor(i, 0, 0, 255);     
          pixels.show();       
          delay (250);           
        }
        pixels.clear();  
        pixels.setPixelColor((NUMPIXELS-1), 0, 0, 255);     
        pixels.show();   
        delay (150);       
     }     


    // ---- animated - red circled complex pattern
     if (opMode == 122) {       
        pixels.clear();        
        for (int i = 0; i < (NUMPIXELS-1); i = i + 1) {           
          pixels.setPixelColor(i, 255, 0, 0);     
          pixels.show();       
          delay (300);           
        }
        pixels.setPixelColor((NUMPIXELS-1), 255, 0, 0);     
        pixels.show();    
        delay (100);   
     }  

    // ---- animated - green circled complex pattern
     if (opMode == 123) {       
        pixels.clear();        
        for (int i = 0; i < (NUMPIXELS-1); i = i + 1) {           
          pixels.setPixelColor(i, 0, 255, 0);     
          pixels.show();       
          delay (300);           
        }
        pixels.setPixelColor((NUMPIXELS-1), 0, 255, 0);     
        pixels.show();    
        delay (100);   
     }  
          
    // ---- animated - blue circled complex pattern
     if (opMode == 124) {       
        pixels.clear();        
        for (int i = 0; i < (NUMPIXELS-1); i = i + 1) {           
          pixels.setPixelColor(i, 0, 0, 255);     
          pixels.show();       
          delay (300);           
        }
        pixels.setPixelColor((NUMPIXELS-1), 0, 0, 255);     
        pixels.show();    
        delay (100);   
     }  

    // ---- animated - white circled complex pattern
     if (opMode == 130) {       
        pixels.clear();        
        for (int i = 0; i < (NUMPIXELS-1); i = i + 1) {           
          pixels.setPixelColor(i, 255, 255, 255);     
          pixels.show();       
          delay (300);           
        }
        pixels.setPixelColor((NUMPIXELS-1), 255, 255, 255);     
        pixels.show();    
        delay (100);   
     }  

    // ---- animated - white driving circle
     if (opMode == 180) {       
        pixels.clear();        
        for (int i = 1; i < (NUMPIXELS-1); i = i + 1) {   
          pixels.clear();           
          pixels.setPixelColor(i, 255, 255, 255);     
          pixels.show();       
          delay (200);           
        }
        pixels.clear();  
        pixels.setPixelColor((NUMPIXELS-1), 255, 255, 255);     
        pixels.show();    
        delay (100);   
     }  

    // ---- animated - red windmill
     if (opMode == 132) {       
        pixels.clear();        
        pixels.setPixelColor(0, 255, 0, 0); 
        pixels.setPixelColor(1, 255, 0, 0); 
        pixels.setPixelColor(4, 255, 0, 0); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 255, 0, 0); 
        pixels.setPixelColor(2, 255, 0, 0); 
        pixels.setPixelColor(5, 255, 0, 0); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 255, 0, 0); 
        pixels.setPixelColor(3, 255, 0, 0);  
        pixels.setPixelColor(6, 255, 0, 0); 
        pixels.show();          
        delay (200);   
     }  


    // ---- animated - green windmill
     if (opMode == 133) {       
        pixels.clear();        
        pixels.setPixelColor(0, 0, 255, 0); 
        pixels.setPixelColor(1, 0, 255, 0); 
        pixels.setPixelColor(4, 0, 255, 0); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 0, 255, 0); 
        pixels.setPixelColor(2, 0, 255, 0); 
        pixels.setPixelColor(5, 0, 255, 0); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 0, 255, 0); 
        pixels.setPixelColor(3, 0, 255, 0); 
        pixels.setPixelColor(6, 0, 255, 0); 
        pixels.show();          
        delay (200);   
     }  

    // ---- animated - blue windmill
     if (opMode == 134) {       
        pixels.clear();        
        pixels.setPixelColor(0, 0, 0, 255); 
        pixels.setPixelColor(1, 0, 0, 255); 
        pixels.setPixelColor(4, 0, 0, 255); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 0, 0, 255); 
        pixels.setPixelColor(2, 0, 0, 255); 
        pixels.setPixelColor(5, 0, 0, 255); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 0, 0, 255); 
        pixels.setPixelColor(3, 0, 0, 255); 
        pixels.setPixelColor(6, 0, 0, 255); 
        pixels.show();          
        delay (200);   
     }  

    // ---- animated - yellow windmill
     if (opMode == 135) {       
        pixels.clear();        
        pixels.setPixelColor(0, 255, 128, 0); 
        pixels.setPixelColor(1, 255, 128, 0); 
        pixels.setPixelColor(4, 255, 128, 0); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 255, 128, 0); 
        pixels.setPixelColor(2, 255, 128, 0); 
        pixels.setPixelColor(5, 255, 128, 0); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 255, 128, 0); 
        pixels.setPixelColor(3, 255, 128, 0);  
        pixels.setPixelColor(6, 255, 128, 0); 
        pixels.show();          
        delay (200);   
     }  

    // ---- animated - white windmill
     if (opMode == 140) {       
        pixels.clear();        
        pixels.setPixelColor(0, 255, 255, 255); 
        pixels.setPixelColor(1, 255, 255, 255); 
        pixels.setPixelColor(4, 255, 255, 255); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 255, 255, 255); 
        pixels.setPixelColor(2, 255, 255, 255); 
        pixels.setPixelColor(5, 255, 255, 255); 
        pixels.show();  
        delay (300);          
        pixels.clear();        
        pixels.setPixelColor(0, 255, 255, 255); 
        pixels.setPixelColor(3, 255, 255, 255); 
        pixels.setPixelColor(6, 255, 255, 255); 
        pixels.show();          
        delay (200);   
     }  

     
    // ---- animated - solar and grid charging
     if (opMode == 141) {       
        pixels.clear();        
        pixels.setPixelColor(1, 255, 128, 0); 
        pixels.setPixelColor(2, 255, 128, 0);         
        pixels.setPixelColor(4, 0, 0, 255); 
        pixels.setPixelColor(5, 0, 0, 255);         
        pixels.show();  
        delay (300);          

        pixels.clear();        
        pixels.setPixelColor(2, 255, 128, 0); 
        pixels.setPixelColor(3, 255, 128, 0);         
        pixels.setPixelColor(5, 0, 0, 255); 
        pixels.setPixelColor(6, 0, 0, 255);       
        pixels.show();  
        delay (300);          

        pixels.clear();        
        pixels.setPixelColor(3, 255, 128, 0); 
        pixels.setPixelColor(4, 255, 128, 0);         
        pixels.setPixelColor(6, 0, 0, 255); 
        pixels.setPixelColor(1, 0, 0, 255);       
        pixels.show();  
        delay (300);  

        pixels.clear();        
        pixels.setPixelColor(1, 0, 0, 255); 
        pixels.setPixelColor(2, 0, 0, 255);       
        pixels.setPixelColor(4, 255, 128, 0); 
        pixels.setPixelColor(5, 255, 128, 0);         
        pixels.show();  
        delay (300);  

        
        pixels.clear();        
        pixels.setPixelColor(2, 0, 0, 255); 
        pixels.setPixelColor(3, 0, 0, 255);       
        pixels.setPixelColor(5, 255, 128, 0); 
        pixels.setPixelColor(6, 255, 128, 0);         
        pixels.show();  
        delay (300);  
        
        pixels.clear();        
        pixels.setPixelColor(1, 255, 128, 0);          
        pixels.setPixelColor(3, 0, 0, 255); 
        pixels.setPixelColor(4, 0, 0, 255);   
        pixels.setPixelColor(6, 255, 128, 0);        
        pixels.show();          
        delay (200);   
     }  


    // ---- phat red blink
     if (opMode == 152) {               
        pixels.clear();  
        pixels.show();       
        delay (600);           
        pixels.setPixelColor(0, 255, 0, 0);            
        pixels.setPixelColor(2, 255, 0, 0);
        pixels.setPixelColor(4, 255, 0, 0);
        pixels.setPixelColor(6, 255, 0, 0);        
        pixels.show();
        delay (500);         
     }
     
     // ---- custom color set in blink opPattern - center / single - "195:255188000"
     if (opMode == 195) {    
        pixels.setPixelColor(0, 0, 0, 0);     
        pixels.show();  
        delay (600);       
        int col1 = opParameter.substring(0,3).toInt();
        int col2 = opParameter.substring(3,6).toInt();
        int col3 = opParameter.substring(6,9).toInt();            
        pixels.setPixelColor(0, col1, col2, col3);    
        pixels.show();  
        delay (500);               
     }

     // ---- custom color set in blink opPattern - all / full - "196:255188000"
     if (opMode == 196) {          
        uint32_t clr1 = pixels.Color(0, 0, 0);  
        pixels.fill(clr1, 0, 7);    
        pixels.show();  
        delay (600);          
        int col1 = opParameter.substring(0,3).toInt();
        int col2 = opParameter.substring(3,6).toInt();
        int col3 = opParameter.substring(6,9).toInt();      
        uint32_t clr = pixels.Color(col1, col2, col3);  
        pixels.fill(clr, 0, 7);      
        pixels.show();  
        delay (500);     
     }



    
  } // Blink & animated
  
  patternUpdated = 0;
   
} // set LED colors



// ---------------------------------------------- startWiFi -----------------------------------

void startWiFi(boolean showParams = false) {

  wc.setDebug(true);
  wc.setAPCallback(configModeCallback);  
  
  // wc.resetSettings();                        //helper to remove the stored wifi connection, comment out after first upload and re upload
  // wc.resetSettings();    

  // set the wifi hotspot pattern
   pixels.clear();
   pixels.setBrightness(brightness);
   uint32_t magenta = pixels.Color(255, 0, 255);  
   pixels.fill(magenta, 0, 7);
   pixels.show();
   
  // check wifi
  const char* apName = "PowerPlant";
  if (!wc.autoConnect()) {                      // try to connect to wifi
    wc.startConfigurationPortal(AP_WAIT);       // if not connected show the configuration portal
  }
  
}

void configModeCallback(WiFiConnect *mWiFiConnect) {
  Serial.println("Entering Access Point Mode.");
}


// ---------------------------------------------- setup -----------------------------------

void setup() {

  Serial.begin(115200);

   // ------------------------ INIT NEOPIXEL ----------------------------
   
  pinMode(NEOPIXEL_PIN, OUTPUT);

  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif

  pixels.begin();
  pixels.clear();
  pixels.setBrightness(brightness);

  // startup Pattern
  pixels.setPixelColor(0, 0, 0, 0, 255);
  pixels.setPixelColor(2, 0, 0, 0, 255);
  pixels.setPixelColor(4, 0, 0, 0, 255);
  pixels.show();
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

  startWiFi();

  // ------------------------ INIT MQTT ----------------------------

  mqtt_client.begin("sono.community", wifi_client);
  mqtt_client.onMessage(messageReceived);  
  connectToMQTT();

  // startup completed - display waiting signal
  pixels.setBrightness(brightness);
  pixels.setPixelColor(0, 255, 255, 255);  
  pixels.show();
  
}

// ---------------------------------------------- loop -----------------------------------

void loop() {

  mqtt_client.loop();
  delay(100);

  updatePixelPattern();

  temp_monitor_count++;
  if (temp_monitor_count > 36000) {
    Serial.print("Temperature: ");
    Serial.print((temprature_sens_read() - 32) / 1.8);
    Serial.println(" C");  

    float floatyC = (temprature_sens_read() - 32) / 1.8;

    String messageStr = "Temp: " + String((int) (floatyC)) + " Device: " + myUUID + " X";
    
    //  (int) Math.round (z);
    mqtt_client.publish("/sion_status/powerplant/" + myUUID, messageStr);
    temp_monitor_count = 0;
  }

  // Wifi died? Start Portal Again
  if (WiFi.status() != WL_CONNECTED) {
    if (!wc.autoConnect()) {

      // set the wifi hotspot pattern
      pixels.clear();
      pixels.setBrightness(brightness);
      uint32_t magenta = pixels.Color(255, 0, 255);  
      pixels.fill(magenta, 0, 7);
      pixels.show();
         
      wc.startConfigurationPortal(AP_WAIT);
      
    }
  }

}


// ---------------------------------------------- end -----------------------------------
