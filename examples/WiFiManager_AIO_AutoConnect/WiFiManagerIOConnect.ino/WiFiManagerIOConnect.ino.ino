// Adafruit IO + WiFiManager Example
//
// by Brent Rubell for Adafruit Industries, 2018
//

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
// Adafruit IO WiFi Client
#include "AdafruitIO_WiFi.h"
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

// default values for Adafruit IO
char AIO_KEY[34] = "YOUR_AIO_KEY";
char* AIO_USERNAME = "YOUR_AIO_USERNAME";

char* WIFI_SSID;
char* WIFI_PASS;

//flag for saving data
bool shouldSaveConfig = false;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println("*IO: WiFi SSID");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // wait for serial monitor to open
  while(! Serial);
  
  Serial.println();

  //clean FS, for testing
  SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(AIO_USERNAME, json["AIO_USERNAME"]);
          strcpy(AIO_KEY, json["AIO_KEY"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } 
  else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_AIO_USERNAME("IO Username", "IO USERNAME", AIO_USERNAME, 20);
  WiFiManagerParameter custom_AIO_KEY("IO Key", "Adafruit IO Key", AIO_KEY, 34);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setAPCallback(configModeCallback);

  //set static ip
  wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  // Adafruit IO Param Configuration
  WiFiManagerParameter custom_text("<p>Adafruit IO Configuration (io.adafruit.com)</p>");
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_AIO_USERNAME);
  wifiManager.addParameter(&custom_AIO_KEY);

  //reset settings - for testing
  wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);


  // attempt to connect
  if (!wifiManager.autoConnect("AdafruitIO_Device", "iotforeveryone")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
   }
  
  wifiManager.setAPCallback(configModeCallback);
   
  //if you get here you have connected to the WiFi
  Serial.println("Connected to WiFi");

  wifiManager.setAPCallback(configModeCallback);
  
  //read updated parameters
  strcpy(AIO_USERNAME, custom_AIO_USERNAME.getValue());
  strcpy(AIO_KEY, custom_AIO_KEY.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config to /config.json");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["AIO_USERNAME"] = AIO_USERNAME;
    json["AIO_KEY"] = AIO_KEY;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  // br: we'll expose this in the library later, but copy over ssid and pass
  WiFi.SSID().toCharArray(WIFI_SSID, 20);
  WiFi.psk().toCharArray(WIFI_PASS, 20);
  
  // disconnect from the prev. wifi manager, use AIO manager. fix this in later release.
  Serial.println("disconnecting from network");
  WiFi.disconnect();

  Serial.print("Connecting to Adafruit IO");
  setupAdafruitIO();
}

void setupAdafruitIO() {
  // Set up Adafruit IO Connection, after WiFiManager portal finishes.
  
  Serial.println("*IO DBG*");
  Serial.println(AIO_USERNAME);
  Serial.println(AIO_KEY);
  Serial.println(WIFI_SSID);
  Serial.println(WIFI_PASS);
  delay(1000);

  // Create AdafruitIO_WiFi `io` instance
  AdafruitIO_WiFi io(AIO_USERNAME, AIO_KEY, WIFI_SSID, WIFI_PASS);
    
  // connect to io.adafruit.com
  Serial.println("Connecting to Adafruit IO");

  // set up the `example` feed
  AdafruitIO_Feed *example = io.feed("example");

  // connect to Adafruit IO
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}

void loop() {
  // put your main code here, to run repeatedly:

}
