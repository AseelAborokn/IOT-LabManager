#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include "StationConfigsManager.h"
#include "RESTfulClient.hpp"
#include "FirebaseConnection.hpp"
#include "utils.h"

extern bool setupTime();
extern int calcSecsLeft(String time_end);
extern bool updateFieldsFromFirebase();
extern int timeFinishSetup;

//LCD initialize
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

//Scan Card ID Button initialize
#define SCAN_PIN      12                   //D12

//Beeper initialize
#define BEEPER_PIN    33                   //D33

//Emergency Shutdown Button initialize
#define SHUTDOWN_PIN   13                  //D13

int beeperTimeInterval = 5;                //in seconds

void setUpLCD() {
  lcd.init();                   
  lcd.backlight();
}

void setup() {
  //Setup LCD screen
  setUpLCD();
  
  //Setup WiFi
  lcd.setCursor(0, 0);
  lcd.print("Setup Station");
  lcd.setCursor(0, 1);
  lcd.print("Configurations!");
  setupStationConfigs();
  delay(5000);
  lcd.clear();
  
  //Setup Time
  setupTime();
  while(timeFinishSetup == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("Getting Time ");
    lcd.setCursor(1, 3);
    lcd.print("Adjustment");
    delay(500);
    onDemandConfigStation();
  }
  lcd.clear();
  
  //Setup Firebase connection
  FirebaseConnection();

  if(!updateFieldsFromFirebase())
  {
    wm.resetSettings();
    delay(3000);
    ESP.restart();
  }
  
  setUpRFID();

  //Set up buttons
  pinMode(SCAN_PIN, INPUT_PULLUP);
  pinMode(SHUTDOWN_PIN, INPUT_PULLUP);

  //Set up beeper and default to silence
  //For Beeper HIGH = Silence, LOW = BEEP
  pinMode(BEEPER_PIN, OUTPUT);
  digitalWrite(BEEPER_PIN, HIGH);         

  //Sonoff Connection setup
  RESTfulClient::initRESTClient();

  lcd.setCursor(0,0);
  lcd.print("Setup completed");
  delay(2000);
  lcd.clear();

}

void loop() {
  // In case we want to reconfigure -> hold button
  if(onDemandConfigStation())
    updateFieldsFromFirebase();

  // Reset the output lights and screen
  lcd.clear();
  turnOnBlueLight();
  hourlyFetch();
  //Emergency shutdown button
  int shutdown_state = digitalRead(SHUTDOWN_PIN);
  if(shutdown_state == LOW) {
    Serial.println("Shutdown button pressed");
    stopUsage();
    return;
  }
  
  //Usage time is over 
  int timeLeft = calcSecsLeft(lastUsageFinishTime);
  if(lastUsageFinishTime.compareTo("") != 0 && timeLeft <= 0)
  {
    stopUsage();
    return;
  }

  //Use time is almost over - Signal with a BEEP
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
    lcd.print("Ready to scan");
    delay(2000);

    if(scanCard() == false) return;
    // There is a valid card present
    
    printUIDToLCD(mfrc522.uid.uidByte, mfrc522.uid.size, 1);
  }
  else {
    if(scanCard() == false) return;
    // There is a valid card present
    
    printUIDToLCD(mfrc522.uid.uidByte, mfrc522.uid.size, 0);
    String userID = FirebaseConnection::getUserID(cidToString(mfrc522.uid.uidByte, mfrc522.uid.size));
    if(userID.compareTo("ERROR") == 0)
    {
      lcd.setCursor(0,0);
      lcd.print("An error has");
      lcd.setCursor(0,1);
      lcd.print("occurred");
      delay(3000);
      return;
    }
    if(userID.compareTo("") == 0)
      userID = "GeneralUser";
    checkUsage(userID);
  }

  // Print the result for 5 secs!
  delay(5000);
}
