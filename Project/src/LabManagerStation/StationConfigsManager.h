/*
  WiFiManager with saved textboxes Demo
  wfm-text-save-demo.ino
  Saves data in JSON file on ESP32
  Uses SPIFFS
 
  DroneBot Workshop 2022
  https://dronebotworkshop.com
 
  Functions based upon sketch by Brian Lough
  https://github.com/witnessmenow/ESP32-WiFi-Manager-Examples
*/

// Include Libraries
#include <WiFi.h>         // WiFi Library
#include <FS.h>           // File System Library
#include <SPIFFS.h>       // SPI Flash Syetem Library
#include <WiFiManager.h>  // WiFiManager Library
#include <ArduinoJson.h>  // Arduino JSON library

#define ESP_DRD_USE_SPIFFS true                   // ASEEL TODO("Add Exp Here")
#define TRIGGER_PIN 13                             // select which pin will trigger the configuration portal when set to LOW
#define JSON_CONFIG_FILE "/station_configs.json"  // JSON configuration file
#define MAX_ID_LENGTH 128                         // Max number of UID at MongoDB Document
#define MAX_PROFESSION_NUMBER_DIGITS 3            // 0 <= professionLevel <= 999 
#define IS_STATION_FUNCTIONS_INPUT_LENGTH 1       // 0 (FALSE) / 1 (TRUE)

bool shouldSaveConfig = false;  // Flag for saving data
int timeout = 120;              // seconds to run for
 
// Custom Configurations - Global-Variables to hold data from custom textboxes
int stationId = 1234;
int requiredProfessionLevel = 0;
bool isStationFunctions = false;

// 

// Define WiFiManager Object
WiFiManager wm;

void forceConfigPortal() {
  if (!wm.startConfigPortal("StationConfigurationsManager_AP", "password")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
}

/**
 * This method saves the Global-Configuration-Variables in JSON format onto the station_configs file
 */
bool saveConfigFile()
{
  Serial.println(F("Saving configuration..."));
  // Create a JSON document
  StaticJsonDocument<512> json;
  json["stationId"] = stationId;
  json["requiredProfessionLevel"] = requiredProfessionLevel;
  json["isStationFunctions"] = isStationFunctions;

  // Open config file
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");         
  
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


void saveCustomParameters(WiFiManagerParameter* custom_station_id, WiFiManagerParameter* custom_profession_level, WiFiManagerParameter* custom_enabled) {
  /////////Save custom parmeters
  // If we get here, we are connected to the WiFi
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  // Lets deal with the user config values
  
  //stationId
  stationId = atoi(custom_station_id->getValue());
  Serial.print("StationId: ");
  Serial.println(stationId);

  //requiredProfessionLevel
  requiredProfessionLevel = atoi(custom_profession_level->getValue());
  Serial.print("requiredProfessionLevel: ");
  Serial.println(requiredProfessionLevel);

  //isStationFunctions
  isStationFunctions = (bool)(atoi(custom_enabled->getValue()));
  Serial.print("isStationFunctions: ");
  Serial.println(isStationFunctions);
 
 
  // Save the custom parameters to FS
  if (shouldSaveConfig) {
    saveConfigFile();
  }
}
/**
 * This method loads the existing configurations from the station_configs file to the global variables!
 */
bool loadConfigFile()
{
  // Uncomment if we need to format filesystem - in case you have problems with the ESP for the first time
  // SPIFFS.format();
 
  // Read configuration from FS json
  Serial.println("Mounting File System...");
 
  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(false) || SPIFFS.begin(true)) {
    Serial.println("mounted file system");
    
    if (SPIFFS.exists(JSON_CONFIG_FILE)) { // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      
      if (configFile) {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          Serial.println("Parsing JSON");

          // Loading the configs from the file to the global variables
          stationId = json["stationId"].as<int>();
          requiredProfessionLevel = json["requiredProfessionLevel"].as<int>();
          isStationFunctions = json["isStationFunctions"].as<bool>();

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
 * Callback notifying us of the need to save configuration
 * This method is called when we need to save the configurations manually!
 */
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

/**
 * Called when config mode launched
 */
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered Configuration Mode");
 
  Serial.print("Config SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
 
  Serial.print("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
}

/**
 * This method takes the new configurations of the station and update them in the config file
 * int newId - station's new Id number.
 * int newProfessionLevel - station's new minimal professionalizm required to operate it.
 * int enable - 1 if the station functions and 0 in case the station is not operatable.
 */
bool updateConfigFile(int newId, int newProfessionLevel, bool enable) {
  Serial.println(F("Updating configurations..."));
  // Backup the global configs in case of failure
  int oldId = stationId;
  int oldProfessionLevel = requiredProfessionLevel;
  bool oldEnable = isStationFunctions;

  // Change the global configs
  stationId = newId;
  requiredProfessionLevel = newProfessionLevel;
  isStationFunctions = enable;
  
  // update the configs
  bool result = saveConfigFile();

  // Restore the global configs in case of failure
  if(!result) {
    stationId = oldId;
    requiredProfessionLevel = oldProfessionLevel;
    isStationFunctions = oldEnable;
    Serial.println(F("Updating configurations -> FAILED!"));
  }

  Serial.println(F("Updating configurations -> SUCCEEDED!"));
  return result;
}


/**
 * This method demands to configure the station on push of a button connected to GPIO 0
 */
bool onDemandConfigStation() {
  if ( digitalRead(TRIGGER_PIN) == LOW ) { // is configuration portal requested?
    //wm.resetSettings();                 //reset settings - for testing
    wm.setConfigPortalTimeout(timeout); // set configportal timeout
    
    //This Add Section should be the same as setupStationConfigs - if you update here please update that function as well
    // Adding custom elements to the page (corresponds to the global configs)
    // stationId
    char convertedIdValue[MAX_ID_LENGTH];               
    sprintf(convertedIdValue, "%d", stationId); 
    WiFiManagerParameter custom_station_id("station_id", "Station's Id: ", convertedIdValue, MAX_ID_LENGTH);
    //Try(&custom_station_id);
    
    // requiredProfessionLevel
    char convertedLevelValue[MAX_PROFESSION_NUMBER_DIGITS];
    sprintf(convertedLevelValue, "%d", requiredProfessionLevel); 
    WiFiManagerParameter custom_profession_level("custom_profession_level", "Station's Minimal Profession Level Required: ", convertedLevelValue, MAX_PROFESSION_NUMBER_DIGITS); 
    
    // isStationFunctions
    char convertedEnableValue[IS_STATION_FUNCTIONS_INPUT_LENGTH];
    sprintf(convertedEnableValue, "%d", (int)(isStationFunctions)); 
    WiFiManagerParameter custom_enabled("enabled", "Is Station Functions? ", convertedEnableValue, IS_STATION_FUNCTIONS_INPUT_LENGTH); 
  
    //Force config portal
      forceConfigPortal();
    // End Force Config
  
    //Save custom parmeters
    // If we get here, we are connected to the WiFi
    saveCustomParameters(&custom_station_id, &custom_profession_level, &custom_enabled);
    return true;
  }
  return false;
}
 
/**
 * This method is basically the setup() for this unit (call it in setup())
 */
void setupStationConfigs() {
  // Defining GPIO 0 as the trigger-pin for onDemandConfigure
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  
  // Change to true when testing to force configuration every time we run
  bool forceConfig = false;

  // Loading the configs from the 
  bool spiffsSetup = loadConfigFile();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }
  WiFi.mode(WIFI_STA);                            // Explicitly set WiFi mode (Station-Mode / Client-Mode)
  Serial.begin(115200);                           // Setup Serial monitor
//  delay(10);
//  wm.resetSettings();                             // Reset settings (only for development) <---- Comment it out when finishing development!
  wm.setSaveConfigCallback(saveConfigCallback);   // Set config save notify callback
  wm.setAPCallback(configModeCallback);           // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
 
  //This Add Section should be the same as onDemandConfigStation - if you update here please update that function as well
  // Adding custom elements to the page (corresponds to the global configs)
  // stationId
  char convertedIdValue[MAX_ID_LENGTH];               
  sprintf(convertedIdValue, "%d", stationId); 
  WiFiManagerParameter custom_station_id("station_id", "Station's Id: ", convertedIdValue, MAX_ID_LENGTH);
  
  // requiredProfessionLevel
  char convertedLevelValue[MAX_PROFESSION_NUMBER_DIGITS];
  sprintf(convertedLevelValue, "%d", requiredProfessionLevel); 
  WiFiManagerParameter custom_profession_level("custom_profession_level", "Station's Minimal Profession Level Required: ", convertedLevelValue, MAX_PROFESSION_NUMBER_DIGITS); 
  
  // isStationFunctions
  char convertedEnableValue[IS_STATION_FUNCTIONS_INPUT_LENGTH];
  sprintf(convertedEnableValue, "%d", (int)(isStationFunctions)); 
  WiFiManagerParameter custom_enabled("enabled", "Is Station Functions? ", convertedEnableValue, IS_STATION_FUNCTIONS_INPUT_LENGTH); 

          
  // Add all defined parameters
  wm.addParameter(&custom_station_id);
  wm.addParameter(&custom_profession_level);
  wm.addParameter(&custom_enabled);
  // End add Parameter Section
  
  // Run if we need a configuration
  if (forceConfig) {
    ////Force config portal
    forceConfigPortal();
    ////
  }
  else {
    if (!wm.autoConnect("StationConfigurationsManager_AP", "password")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }
  /////////Save custom parmeters
  // If we get here, we are connected to the WiFi
  saveCustomParameters(&custom_station_id, &custom_profession_level, &custom_enabled);
}
