#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include "StationConfigsManager.h"


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



MFRC522 mfrc522(SS_PIN, RST_PIN);         // Create MFRC522 instance

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

void grantAccess() {
  // Printing Access Granted Message on the LCD
  lcd.setCursor(0, 1);
  lcd.print("Access Granted!");
  // Trun Green Light On
  analogWrite(BLUE_PIN, 0);
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 255);
}

void denyAccess() {
  // Printing Access Denied Message on the LCD
  lcd.setCursor(0,1);
  lcd.print("Access Denied!");
  // Trun Red Light On
  analogWrite(BLUE_PIN, 0);
  analogWrite(RED_PIN, 255);
  analogWrite(GREEN_PIN, 0);
}
void setup() {
  setupStationConfigs();
//  delay(3000);
//  turnOnBlueLight();
  setUpRFID();
  setUpLCD();

  // In case we want to reconfigure -> hold button + click on restart1
  while(onDemandConfigStation());
}

void loop() {
  // Reset the output lights and screen
  lcd.clear();
  turnOnBlueLight();

  // There is no new present
  if ( ! mfrc522.PICC_IsNewCardPresent()) return;
  // 
  if ( ! mfrc522.PICC_ReadCardSerial()) return;

  // There is a valid card present
  // Check if card has access or not.
  //TODO: This is a simple example, when we create Database, will send a request and recieve answer instead of using b1 and a loop.
  byte b1[] = {0x6b, 0x8f, 0xd5, 0xab};
  byte* b2;
  b2 = mfrc522.uid.uidByte;
  int size = mfrc522.uid.size;
  int j = 0;
  byte f = byte(15);
  byte zero = 0;
  for(int i = 0; i<4; i++)
  {
    if(b2[i] <= f && b2[i] >= zero)
    {
      lcd.setCursor(j,0);
      lcd.print(0);
      lcd.setCursor(j+1,0);
      lcd.print(b2[i], HEX);
    }
    else
    {
      lcd.setCursor(j,0);
      lcd.print(b2[i], HEX);  
    }
    j+=3;
  }

  // b2 is the scanned card
  delay(1000);
  bool isEqual = true;
  for(int i = 0; i<4; i++)
  {
    if(b1[i] != b2[i])
      isEqual = false;
  }

  // Check if we need to grand the access or deny it.
  if(isEqual){
    grantAccess();
    //Serial.println("ACCESS");
  }
  else {
    denyAccess();
    //Serial.println("NO ACCESS");
  }

  // Print the result for 5 secs!
  delay(5000); 
}
