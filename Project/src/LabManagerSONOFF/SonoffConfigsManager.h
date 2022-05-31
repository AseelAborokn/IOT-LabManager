/*
  WiFiManager with saved textboxes Demo
  wfm-text-save-demo.ino
  Saves data in JSON file on ESP32
  Uses LittleFS
  DroneBot Workshop 2022
  https://dronebotworkshop.com
  Functions based upon sketch by Brian Lough
  https://github.com/witnessmenow/ESP32-WiFi-Manager-Examples
*/

// Include Libraries
#include <ESP8266WiFi.h>  // WiFi Library
#include <FS.h>           // File System Library
#include <LittleFS.h>     // SPI Flash Syetem Library
#include <WiFiManager.h>  // WiFiManager Library
#include <ArduinoJson.h>  // Arduino JSON library

#define ESP_DRD_USE_LittleFS true            
#define TRIGGER_PIN 0                             // select which pin will trigger the configuration portal when set to LOW
#define JSON_CONFIG_FILE "/sonoff_configs.json"   // JSON configuration file
#define MAX_ID_LENGTH 128                         // Max number of UID at MongoDB Document
#define MAX_ADDRESS_LENGTH 16                     // Max number of UID at MongoDB Document

bool shouldSaveConfig = false;  // Flag for saving data
int timeout = 120;              // seconds to run for

// Custom Configurations - Global-Variables to hold data from custom textboxes
String myMacAddress;                                  // SONOFF MacAddress
char static_ip[MAX_ADDRESS_LENGTH] = "10.0.1.56";     // Static IP Address
char static_gw[MAX_ADDRESS_LENGTH] = "10.0.1.1";      // Static gateway
char static_sn[MAX_ADDRESS_LENGTH] = "255.255.0.0";   // Static Subnet
IPAddress primaryDNS(8, 8, 8, 8);                     //DO NOT CHANGE UNLESS YOU KNOW HOW TO - This is relevant to HTTP requests

// Define WiFiManager Object
WiFiManager wm;

void forceConfigPortal() {
  if (!wm.startConfigPortal("SwitchConfigurationsManager_AP", "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }
}

/**
   This method saves the Global-Configuration-Variables in JSON format onto the sonoff_configs file
*/
bool saveConfigFile()
{
  Serial.println(F("Saving configuration..."));
  // Create a JSON document
  StaticJsonDocument<512> json;
  json["myMacAddress"] = myMacAddress;
  json["ip"] = WiFi.localIP().toString();
  json["gateway"] = WiFi.gatewayIP().toString();
  json["subnet"] = WiFi.subnetMask().toString();
  
  // Open config file
  File configFile = LittleFS.open(JSON_CONFIG_FILE, "w");

  if (!configFile) { // Error, file did not open
    Serial.println(F("Updating configurations -> FAILED with (Failed to open config file for writing)!"));
    return false;
  }

  // Serialize JSON data to write to file
  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0) { // Error writing file
    Serial.println(F("Updating configurations -> FAILED with (Failed to write to file)!"));
    return false;
  }

  configFile.close(); // Close file
  Serial.println(F("Saving configurations -> SUCCEEDED!"));
  return true;
}


void saveCustomParameters(WiFiManagerParameter* customMacAddress) {
  // If we get here, we are connected to the WiFi
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address (WiFi.localIP): ");
  Serial.println(WiFi.localIP());

  // Lets deal with the user config values
  //MAC Address of Sonoff Switch
  myMacAddress = String(customMacAddress->getValue());
  Serial.print("myMacAddress: ");
  Serial.println(myMacAddress);
  // Static IP Address
  strcpy(static_ip, WiFi.localIP().toString().c_str());
  Serial.print("IP Address: ");
  Serial.println(static_ip);
  // Static Gateway
  strcpy(static_gw, WiFi.gatewayIP().toString().c_str());
  Serial.print("Gateway Address: ");
  Serial.println(static_gw);
  // Static Subnet
  strcpy(static_sn, WiFi.subnetMask().toString().c_str());
  Serial.print("Subnet Address: ");
  Serial.println(static_sn);

  // Save the custom parameters to FS
  if (shouldSaveConfig) {
    saveConfigFile();
  }
}
/**
   This method loads the existing configurations from the sonoff_configs file to the global variables!
*/
bool loadConfigFile()
{
  // Uncomment if we need to format filesystem - in case you have problems with the ESP for the first time
  // LittleFS.format();

  // Read configuration from FS json
  Serial.println("Mounting File System...");

  // May need to make it begin(true) first time you are using LittleFS
  if (LittleFS.begin()) {
    Serial.println("mounted file system");

    if (LittleFS.exists(JSON_CONFIG_FILE)) { // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = LittleFS.open(JSON_CONFIG_FILE, "r");

      if (configFile) {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          Serial.println("Parsing JSON");

          // Loading the configs from the file to the global variables
          myMacAddress = json["myMacAddress"].as<String>();
          if (json["ip"]) {
            Serial.println("setting custom ip from config");
            strcpy(static_ip, json["ip"].as<String>().c_str());
            strcpy(static_gw, json["gateway"].as<String>().c_str());
            strcpy(static_sn, json["subnet"].as<String>().c_str());
            Serial.println(static_ip);
          } else {
            Serial.println("no custom ip in config");
          }
          Serial.println(F("Mounting File System -> SUCCEEDED!"));
          return true;
        }
        else { // Error loading JSON data
          Serial.println(F("Mounting File System -> FAILED with (Failed to load json config)"));
        }
      }
    }
  }
  else { // Error mounting file system
    Serial.println(F("Mounting File System -> FAILED with (Failed to mount FS)"));
  }

  Serial.println(F("Mounting File System -> FAILED!"));
  return false;
}

/**
   Callback notifying us of the need to save configuration
   This method is called when we need to save the configurations manually!
*/
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

/**
   Called when config mode launched
*/
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered Configuration Mode");

  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());

  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

/**
   This method demands to configure the station on push of a button connected to GPIO 0
*/
bool onDemandConfigSONOFF() {
  if ( digitalRead(TRIGGER_PIN) == LOW ) { // is configuration portal requested?
    Serial.println("Resetting SONOFF For Demand Configurations.");
    wm.resetSettings();                 //reset settings - for testing
    delay(3000);
    ESP.restart();
    return true;
  }
  return false;
}

/**
   This method is basically the setup() for this unit (call it in setup())
*/
void setupSONOFFConfigs() {
  Serial.begin(115200);                           // Setup Serial monitor
  delay(8000);
  // Defining GPIO 0 as the trigger-pin for onDemandConfigure
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  // Change to true when testing to force configuration every time we run
  bool forceConfig = false;

  // Loading the configs from the
  bool LittleFSSetup = loadConfigFile();
  if (!LittleFSSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }
  WiFi.mode(WIFI_STA);                            // Explicitly set WiFi mode (Station-Mode / Client-Mode)
  //MAC Address
  myMacAddress = WiFi.macAddress();

  //  wm.resetSettings();                             // Reset settings (only for development) <---- Comment it out when finishing development!
  wm.setSaveConfigCallback(saveConfigCallback);   // Set config save notify callback
  wm.setAPCallback(configModeCallback);           // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  
  // Adding custom elements to the page (corresponds to the global configs)
  // MAC Address
  char convertedMACValue[MAX_ID_LENGTH];
  sprintf(convertedMACValue, "%s", myMacAddress.c_str());
  WiFiManagerParameter custom_MAC_Address("myMacAddress", "Sonoff's MAC Address: ", convertedMACValue, MAX_ID_LENGTH, "readonly");
  // Creating the custome IP network
  IPAddress _ip, _gw, _sn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  wm.setSTAStaticIPConfig(_ip, _gw, _sn, primaryDNS);
  
  // Add all defined parameters
  wm.addParameter(&custom_MAC_Address);
  // End add Parameter Section

  // Run if we need a configuration
  if (forceConfig) {
    ////Force config portal
    forceConfigPortal();
  }
  else {
    if (!wm.autoConnect("SwitchConfigurationsManager_AP", "password")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }
  /////////Save custom parmeters
  // If we get here, we are connected to the WiFi
  saveCustomParameters(&custom_MAC_Address);
}
