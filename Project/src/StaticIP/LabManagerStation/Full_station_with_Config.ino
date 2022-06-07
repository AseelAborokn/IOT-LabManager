#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include "StationConfigsManager.h"
#include "RESTfulClient.hpp"
//#include "TimeCalc.h"
#include "FirebaseConnection.hpp"

extern String printLocalTime();
extern char* stringToCharArr(String st);
extern void setupTime();
extern int calcSecsLeft(char* time_end);
extern String sonoff_ip_address;

//LCD initalize
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

//RFID initalize
#define RST_PIN         4
#define SS_PIN          5

//Light initalize
#define RED_PIN          25                //D25
#define GREEN_PIN        26                //D26
#define BLUE_PIN         27                //D27

//Scan Card ID Button initalize
#define SCAN_PIN      12                //D12

//Beeper GPIO initalize
#define BEEPER_PIN    33                //D33

//Emergency Shutdown Button initialize
#define SHUTDOWN_PIN   13              //D13

//Initalize last usage parameters
String lastScannedUID = "";
String lastUsageDocumentName = "";
String lastUsageFinishTime = "";

int beeperTimeInterval = 5; //in seconds

MFRC522 mfrc522(SS_PIN, RST_PIN);         // Create MFRC522 instance

extern bool updateFieldsFromFirebase();

void setUpRFID() {
  while (!Serial);                        // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();                            // Init SPI bus
  mfrc522.PCD_Init();                     // Init MFRC522
  delay(4);                               // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();      // Show details of PCD - MFRC522 Card Reader details
}

void turnOnBlueLight() {
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 255);
}

void setUpLCD() {
  lcd.init();                   
  lcd.backlight();
}

void extendAccess() {
  //Printing to LCD
  lcd.setCursor(0,0);
  lcd.print("Usage Extended");
  //update in firebase
  FirebaseConnection::extendUsageRecord(lastUsageDocumentName, run_time_in_secs);
  //update last user finish time
  lastUsageFinishTime = FirebaseConnection::getUseDocField(lastUsageDocumentName, "timestamp", "finished_at");
}

void grantAccess(String UIDScanner) {
  Serial.println("ACCESS GRANTED");
  // Printing Access Granted Message on the LCD
  lcd.setCursor(0, 1);
  lcd.print("Access Granted!");
  // Trun Green Light On
  analogWrite(BLUE_PIN, 0);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 255);
  
  //send an HTTP request to turn on sonoff smart switch
  String hostName = sonoff_ip_address;
  String path = "relay/on";
  RESTfulClient::urlGET(hostName, path);

  //Update usage in Firebase
  Serial.println("Updating in Usage History");
  String timeStarted = printLocalTime();
  lastUsageDocumentName = FirebaseConnection::recordUsageInHistory(station_id, UIDScanner, stringToCharArr(timeStarted), run_time_in_secs);
  lastScannedUID = UIDScanner;
  lastUsageFinishTime = FirebaseConnection::getUseDocField(lastUsageDocumentName, "timestamp", "finished_at");
  delay(5000);
}

void denyAccess() {
  Serial.println("ACCESS DENIED");
  // Printing Access Denied Message on the LCD
  lcd.setCursor(0,1);
  lcd.print("Access Denied!");
  // Trun Red Light On
  analogWrite(BLUE_PIN, 0);
  analogWrite(RED_PIN, 255);
  analogWrite(GREEN_PIN, 0);
}

//Turn UID from char array to String
String cidToString(byte* b2, int b2_size) //TODO:: check
{
  byte f = byte(15);
  byte zero = 0;
  String st = "";
  for(int i = 0; i<b2_size; i++)
  {
    if(b2[i] <= f && b2[i] >= zero)
    {
      st += String(0, HEX);
    }
    st += String(b2[i], HEX);
  }
  Serial.println("CID IS ");
  Serial.println(st);
  return st;
}
//prints the UID to the LCD 
void printUIDToLCD(byte* b2, int b2_size, int row)
{
  int j = 0;
  byte f = byte(15);
  byte zero = 0;
  for(int i = 0; i<b2_size; i++)
  {
    if(b2[i] <= f && b2[i] >= zero)
    {
      lcd.setCursor(j,row);
      lcd.print(0);
      lcd.setCursor(j+1,row);
      lcd.print(b2[i], HEX);
    }
    else
    {
      lcd.setCursor(j,row);
      lcd.print(b2[i], HEX);  
    }
    j+=3;
  }
}

void checkUsage(String UIDScanner) {
  //Station is unavailable
  if(station_status.compareTo("unavailable") == 0)
  {
    Serial.println("Station is unavailable - Deny access");
    denyAccess();
    return;
  }
  if(lastUsageFinishTime.compareTo("") != 0 && UIDScanner.compareTo(lastScannedUID) != 0) //Trying to access used station, not user
  {
    Serial.print("Station is in use until ");
    Serial.print(lastUsageFinishTime);
    Serial.print(" - Deny access");
    denyAccess();
    return;
  }
  //Station is available
  if(lastScannedUID.compareTo(UIDScanner) == 0) //User wants to extend usage
  {
    Serial.println("Request for extended public station access - Extend access");
    extendAccess();
    return; 
  }
  String permission_status = FirebaseConnection::getPermissionStatus(station_id, owner_id, UIDScanner);
  if(accessibility.compareTo("public") == 0) //Station is public
  {
    if(permission_status.compareTo("") != 0 && permission_status.compareTo("granted") != 0) //User was denied access
    {
      Serial.println("User was denied access to public station - Deny access");
      denyAccess();
      return;
    }
    else
    {//New user, not denied acces, grant access and create usage history
      Serial.println("Station is public and available for usage - Grant access");
      grantAccess(UIDScanner);
      return;
    }
  }
  //Station is available and private
  if(owner_id.compareTo(UIDScanner) == 0) // Scanner is owner
  {
    Serial.println("Owner accesses station - grant access");
    grantAccess(UIDScanner);
    return;
  }
  //Station is available and private, scanner is not owner or previous user
  if(permission_status.compareTo("granted") == 0) //Has access
  {
    Serial.println("Private, available station, user has permission - grant access");
    grantAccess(UIDScanner);
    return; 
  }
  Serial.println("Private, available station, user does not have permission - deny access");
  denyAccess();//Deny
}
void setup() {
  //Setup WiFi
  setupStationConfigs();
  delay(5000);
  //Setup Time
  setupTime();
  
  //Setup Firebase connection
  FirebaseConnection();

  if(!updateFieldsFromFirebase())
    ESP.restart();
  
  setUpRFID();
  setUpLCD();
  pinMode(SCAN_PIN, INPUT_PULLUP);
  pinMode(SHUTDOWN_PIN, INPUT_PULLUP);
  pinMode(BEEPER_PIN, OUTPUT);
  digitalWrite(BEEPER_PIN, HIGH);

  //Sonoff Connection setup
  RESTfulClient::initRESTClient();
  
}

void loop() {
  // In case we want to reconfigure -> hold button
  if(onDemandConfigStation())
    updateFieldsFromFirebase();

  // Reset the output lights and screen
  lcd.clear();
  turnOnBlueLight();
  
  //Emergency shutdown button
  int shutdown_state = digitalRead(SHUTDOWN_PIN);
  if(shutdown_state == LOW) {
    Serial.println("Shutdown button pressed");
    //Send a request to turn off Sonoff Smart Socket elec. GET HTTP request of relay OFF
    String hostName = sonoff_ip_address;
    String path = "relay/off";
    RESTfulClient::urlGET(hostName, path);
    delay(5000);
    return;
  }

  //Usage time is over
  int timeLeft = calcSecsLeft(stringToCharArr(lastUsageFinishTime));
  if(lastUsageFinishTime.compareTo("") != 0 && timeLeft <= 0)
  {
    //TODO:: turn off switch - move to another method
    //send an HTTP request to turn off sonoff smart switch
    String hostName = sonoff_ip_address;
    String path = "relay/off";
    RESTfulClient::urlGET(hostName, path);
    
    lastUsageFinishTime = "";
    lastScannedUID = "";
    lastUsageDocumentName = "";
    return;
  }
  if(lastUsageFinishTime.compareTo("") != 0 && timeLeft >= 0 && timeLeft <= beeperTimeInterval)
  {
    digitalWrite(BEEPER_PIN, LOW);//BEEP
    delay(500);
    digitalWrite(BEEPER_PIN, HIGH);//STOP BEEP
  }
  //Scan card without sending a request 
  int button_state = digitalRead(SCAN_PIN);
  if(button_state == LOW) {
    lcd.setCursor(0, 0);
    delay(2000);
    // There is no new present
    if ( ! mfrc522.PICC_IsNewCardPresent()) return;
    //
    if ( ! mfrc522.PICC_ReadCardSerial()) return;
    
    byte* b2;
    b2 = mfrc522.uid.uidByte;
    int b2_size = mfrc522.uid.size;
    printUIDToLCD(b2, b2_size, 1);
  }
  else {
    // There is no new present
    if ( ! mfrc522.PICC_IsNewCardPresent()) return;
    // 
    if ( ! mfrc522.PICC_ReadCardSerial()) return;
    // There is a valid card present
    // Check if card has access.
    byte* b2;
    b2 = mfrc522.uid.uidByte;
    int b2_size = mfrc522.uid.size;
    printUIDToLCD(b2, b2_size, 0);
    checkUsage(FirebaseConnection::getUserID(cidToString(b2, b2_size)));
  }

  // Print the result for 5 secs!
  delay(5000);
}
