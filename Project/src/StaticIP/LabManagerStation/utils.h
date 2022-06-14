extern String printLocalTime();
extern String sonoff_ip_address;
extern void turnOnRedLight();
extern void turnOnGreenLight();
extern LiquidCrystal_I2C lcd;
extern bool updateFieldsFromFirebase();

//Initalize last usage parameters
String lastScannedUID = "";
String lastUsageDocumentName = "";
String lastUsageFinishTime = "";

//Does the station need to be updated
bool shouldUpdate = true;

/*
 * Updates global station variables according to Firebase information 
 * Updates them twice an hour
 */
void hourlyFetch() 
{
  String time_now = printLocalTime();
  struct tm timeinfo;
  strptime(time_now.c_str(), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  if(timeinfo.tm_min%30 == 0 && shouldUpdate == true)
  {
    Serial.println("30 Minute Fetch");
     shouldUpdate = !updateFieldsFromFirebase();
  }
  if(timeinfo.tm_min%30 != 0 && shouldUpdate == false)
  {
    shouldUpdate = true;
  }
}

/*
 * Request to extend usage
 * Update Firebase and apropriate variables
 * Print a message to LCD
 */
void extendAccess() {
  //Printing to LCD
  lcd.setCursor(0,0);
  lcd.print("Usage Extended");
  
  //update in firebase
  FirebaseConnection::extendUsageRecord(lastUsageDocumentName, run_time_in_secs);
  
  //update last user finish time in global variable
  lastUsageFinishTime = FirebaseConnection::getUseDocField(lastUsageDocumentName, "timestamp", "finished_at");
}

/*
 * Grant station usage access to scanner
 * in success:   
 *    Turn on green lights and print apropriate message to LCD
 *    Log usage in Firebase
 * in failure: (Due to failure to communicate with Smart Switch
 *    Print appropriate message to LCD
 *    Don't log usage in Firebase
 */
void grantAccess(String UIDScanner) {
  //send an HTTP request to turn on sonoff smart switch
  String hostName = sonoff_ip_address;
  String path = "relay/on";
  bool result = RESTfulClient::urlGET(hostName, path);
  
  //Could not send HTTP request to smart switch, print error message to LCD and do not Log it in Firestore
  if(result == false) 
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Smart Switch is");
    lcd.setCursor(0,1);
    lcd.print("not connected");
    return;
  }
  
  // Printing Access Granted Message on the LCD
  lcd.setCursor(0, 1);
  lcd.print("Access Granted!");
  
  // Trun Green Light On
  turnOnGreenLight();

  //Record usage in Firebase
  Serial.println("Updating in Usage History");
  String timeStarted = printLocalTime();
  lastUsageDocumentName = FirebaseConnection::recordUsageInHistory(station_id, UIDScanner, (timeStarted), run_time_in_secs);
  lastScannedUID = UIDScanner;
  lastUsageFinishTime = FirebaseConnection::getUseDocField(lastUsageDocumentName, "timestamp", "finished_at");
//  delay(5000);
}

/*
 * Deny station usage to scanner
 * Print appropriate message to LCD and turn on Red Light 
 */
void denyAccess() {
  // Printing Access Denied Message on the LCD
  lcd.setCursor(0,1);
  lcd.print("Access Denied!");
  
  // Trun Red Light On
  turnOnRedLight();
}

/*
 * When user's usage time has ended, and no card was scanned for extension
 * Send an HTTP request to Smart Switch to shut off and update variables accordingly
 * in failure:
 *    Print message to LCD
 */
void stopUsage() {
  //send an HTTP request to turn off sonoff smart switch
  String hostName = sonoff_ip_address;
  String path = "relay/off";
  Serial.print("Request is ");
  Serial.println(hostName + " " + path);
  bool result = RESTfulClient::urlGET(hostName, path);

  //Could not send HTTP request to smart switch, print error message to LCD
  if(result == false)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Smart Switch is");
    lcd.setCursor(0,1);
    lcd.print("not connecting");
    delay(3000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Please contact");
    lcd.setCursor(4,1);
    lcd.print("manager"); 
    delay(3000);
  }
  Serial.println("Updating global variables");
  //Remove Last usage from global variables
  lastUsageFinishTime = "";
  lastScannedUID = "";
  lastUsageDocumentName = "";
  return;
}

/*
 * Turns the CID to a string
 */
String cidToString(byte* b2, int b2_size)
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

/*
 * Prints the UID to the LCD at row 
 *    row is in {1,2}
 */ 
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
    j+=2;
  }
}

/*
 * Check if UIDScanner (The User ID of the card scanner):
 *    has access to use station and thus should call grantAccess
 *    was denied access to station and should call denyAccess
 *    is requesting an extension and should call extendAccess
 */
void checkUsage(String UIDScanner) {
  Serial.println("Checking usage for ");
  Serial.println(UIDScanner);
  if(UIDScanner.compareTo("") == 0)
  {
    Serial.println("User not found");
    denyAccess();
    return;
  }
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
    Serial.print("Last UID usage : ");
    Serial.println(lastScannedUID);
    Serial.print("Scanning now : ");
    Serial.println(UIDScanner);
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
  Serial.println("Owner ID is :");
  Serial.println(owner_id);
  if(permission_status.compareTo("granted") == 0) //Has access
  {
    Serial.println("Private, available station, user has permission - grant access");
    grantAccess(UIDScanner);
    return; 
  }
  Serial.println("Private, available station, user does not have permission - deny access");
  denyAccess();//Deny
}
