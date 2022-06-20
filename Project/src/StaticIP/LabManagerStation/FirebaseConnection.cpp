#include "TimeCalc.h"
#include <addons/TokenHelper.h>
#include "FirebaseConnection.hpp"

//Firebase credentials - could replace with adding them as parameters to constructor
#define API_KEY "AIzaSyAS22oKopW8qvwBhUy71To_Egpoe_SHxzw"
#define FIREBASE_PROJECT_ID "iot-labmanager"
#define USER_EMAIL "admin@gmail.com"//"lara.kinan13@gmail.com" //TODO:: remove rules of Firebase so that not to need email
#define USER_PASSWORD "admin123"// "Lara230900"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

extern String station_id;
extern String sonoff_ip_address;
extern bool updateConfigFile(String newOwnerID, String newStationName, int newRunTime, String newAccessibility, String newStationStatus, String newSonoffIpAddress);

/*
 * Cuts the String content to get String after pathToName
 */
String cutName(String content, String pathToName) 
{
  if(content.compareTo(" ") == 0)
    return "";
  int index = content.indexOf(pathToName) + pathToName.length();
  return (content.c_str()+index);
}

bool updateFieldsFromFirebase()
{
  Serial.println("GETTING STATION DOCUMENT");
  if(station_id.compareTo("") == 0)
    return false;
  String station_document = FirebaseConnection::getStationDoc(station_id);
  if(station_document.compareTo("") == 0) //Check if such a Document exists
    return false;
  
  if(station_id.compareTo("") != 0)
  {
    String new_owner_id = FirebaseConnection::getFieldByPayload(station_document, "string", "owner_id");
    String new_station_name = FirebaseConnection::getFieldByPayload(station_document, "string", "name");
    String new_accessibility = FirebaseConnection::getFieldByPayload(station_document, "string", "accessibility");
    String new_station_status = FirebaseConnection::getFieldByPayload(station_document, "string", "status");
    int new_run_time_in_secs = atoi((FirebaseConnection::getFieldByPayload(station_document, "integer", "run_time_in_secs")).c_str());
    updateConfigFile(new_owner_id, new_station_name, new_run_time_in_secs, new_accessibility, new_station_status, sonoff_ip_address);
  }
  return true;
}

// -------------------------------- IMPLEMENTING PRIVATE Methods --------------------------------

void FirebaseConnection::fcsUploadCallback(CFS_UploadStatusInfo info)
{
    if (info.status == fb_esp_cfs_upload_status_init)
    {
        Serial.printf("\nUploading data (%d)...\n", info.size);
    }
    else if (info.status == fb_esp_cfs_upload_status_upload)
    {
        Serial.printf("Uploaded %d%s\n", (int)info.progress, "%");
    }
    else if (info.status == fb_esp_cfs_upload_status_complete)
    {
        Serial.println("Upload completed ");
    }
    else if (info.status == fb_esp_cfs_upload_status_process_response)
    {
        Serial.print("Processing the response... ");
    }
    else if (info.status == fb_esp_cfs_upload_status_error)
    {
        Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

void FirebaseConnection::addField(FirebaseJson *content, String fieldName, String fieldType, String fieldValue)
{
  content->set("fields/"+fieldName+"/"+fieldType+"Value", fieldValue);
}
void FirebaseConnection::addField(FirebaseJson *content, String fieldName, int fieldValue)
{
  content->set("fields/"+fieldName+"/integerValue", fieldValue);
}

void FirebaseConnection::addFilter(FirebaseJson *query, int index, String fieldName, String fieldValue)
{
  query->set("where/compositeFilter/filters/["+((String)(index))+((String)("]/fieldFilter/field/fieldPath")),fieldName);
  query->set("where/compositeFilter/filters/["+((String)(index))+((String)("]/fieldFilter/op")),"EQUAL");
  query->set("where/compositeFilter/filters/["+((String)(index))+((String)("]/fieldFilter/value/stringValue")),fieldValue);
}

String FirebaseConnection::getField(String documentPath, String fieldType, String fieldName)
{
  if (Firebase.ready())
  {
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())){
      return getFieldByPayload(fbdo.payload(), fieldType, fieldName);
    }
    else
        Serial.println(fbdo.errorReason());
  }
  return "";
}

String FirebaseConnection::getQueryField(String payload, String fieldPath)
{
  FirebaseJsonData content;
  FirebaseJsonArray jsonArr;
  FirebaseJson jsonContent;
  content.getArray<String>(payload, jsonArr); //Query returns an array
  if(jsonArr.size() != 1)
    return "";
  jsonArr.iteratorBegin();
  content.getJSON<String>(jsonArr.valueAt(0).value, jsonContent); //Get first document value
  jsonContent.get(content, fieldPath);//Get specific field from document
  return content.to<String>();
}

void FirebaseConnection::updateDocumentField(String documentPath, String fieldName, String fieldType, String newData)
{
  if (Firebase.ready())
  {
      FirebaseJson content;
      addField(&content, fieldName, fieldType, newData);
      if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), fieldName))
          Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      else
          Serial.println(fbdo.errorReason());
  }
}


// -------------------------------- IMPLEMENTING PUBLIC Methods --------------------------------

String FirebaseConnection::getFieldByPayload(String payload, String fieldType, String fieldName)
{
  FirebaseJsonData content;
  FirebaseJson jsonContent;
  content.getJSON<String>(payload, jsonContent);
  jsonContent.get(content, "fields/"+fieldName+"/"+fieldType+"Value");
  //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
  return content.to<String>().c_str();
}

String FirebaseConnection::getDocumentName(String payload, String pathToName)
{
  if(payload.compareTo("") == 0)
    return "";
  FirebaseJsonData content;
  FirebaseJson jsonContent;
  content.getJSON<String>(payload, jsonContent);
  jsonContent.get(content, "name");
  return cutName(content.to<String>(), pathToName);
}

void FirebaseConnection::updateUsageHistoryRecordField(String doc_name, String fieldName, String fieldType, String newData)
{
  String documentPath = "UsageHistory/"+doc_name;
  updateDocumentField(documentPath, fieldName, fieldType, newData);
}

String FirebaseConnection::getStationField(String doc_name, String fieldType, String fieldName)
{
  String documentPath = "LabStations/"+doc_name;
  Serial.print("Get a station field... ");
  return getField(documentPath, fieldType, fieldName);
}

String FirebaseConnection::getUseDocField(String doc_name, String fieldType, String fieldName)
{
  String documentPath = "UsageHistory/"+doc_name;
  Serial.print("Get a Usage History field... ");
  return getField(documentPath, fieldType, fieldName);
}

String FirebaseConnection::addStationToFirebase(String doc_name, String accessibility, String station_name, String owner_id, int run_time_in_secs, String station_status)
{
  if (Firebase.ready())
  {
      FirebaseJson content;
      String documentPath = "LabStations/"+doc_name;
      addField(&content, "accessibility", "string", accessibility);
      addField(&content, "name", "string", station_name);
      addField(&content, "owner_id", "string", owner_id);
      addField(&content, "run_time_in_secs", run_time_in_secs);
      addField(&content, "status", "string", station_status);
      Serial.print("Create a station document... ");

      if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw()))
      {
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        return getDocumentName(fbdo.payload(), "LabStations/");;
      }
      else
          Serial.println(fbdo.errorReason());
  }
  return "";
}

String FirebaseConnection::recordUsageInHistory(String station_name, String user_id, String start_time, int use_time)
{
  if (Firebase.ready())
  {
      FirebaseJson content;
      String documentPath = "UsageHistory/";
      addField(&content, "finished_at", "timestamp", finishTime(start_time, use_time));
      addField(&content, "started_at", "timestamp", start_time);
      addField(&content, "station_id", "string", station_name);
      addField(&content, "user_id", "string", user_id);

      Serial.print("Create a usage document... ");

      if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw()))
          Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      else
          return fbdo.errorReason();
      return getDocumentName(fbdo.payload(), "UsageHistory/");
  }
  return "";
}

void FirebaseConnection::extendUsageRecord(String doc_name, int use_time)
{
  updateUsageHistoryRecordField(doc_name, "finished_at", "timestamp", finishTime((printLocalTime()).c_str(), use_time));
}

String FirebaseConnection::getPermissionStatus(String station_id, String owner_id, String user_id)
{
    if (Firebase.ready())
    {
      Serial.print("Query a Firestore database... ");
      FirebaseJson query;
      query.set("from/collectionId", "Permissions");
      query.set("select/fields/[0]/fieldPath", "permission_status");
      query.set("where/compositeFilter/op", "AND");
      addFilter(&query, 0, "owner_id", owner_id);
      addFilter(&query, 1, "user_id", user_id);
      addFilter(&query, 2, "station_id", station_id);
      if (Firebase.Firestore.runQuery(&fbdo, FIREBASE_PROJECT_ID, "", "" /* The document path */, &query))
      {
        //Print payload to screen
        //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        return getQueryField(fbdo.payload(), "document/fields/permission_status/stringValue");
      }
    
      else
          Serial.println(fbdo.errorReason());
    }
    return "";
}

String FirebaseConnection::getUserID(String cid) {
  if (Firebase.ready())
  {
      Serial.print("Query a Firestore database for UID from CID ... ");
      FirebaseJson query;
      query.set("from/collectionId", "LabUsers");
      query.set("where/compositeFilter/op", "AND");
      addFilter(&query, 0, "cid", cid);
      if (Firebase.Firestore.runQuery(&fbdo, FIREBASE_PROJECT_ID, "", "" /* The document path */, &query))
      {
        //Print payload to screen
        Serial.println("Debugging for cor paniced issue");
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        String content = getQueryField(fbdo.payload(), "document/name");
        return cutName(content, "LabUsers/");
      }
      else
          Serial.println(fbdo.errorReason());
  }
  return "ERROR";
}

String FirebaseConnection::getStationDoc(String station_id)
{
  String documentPath = "LabStations/"+station_id;
  if (Firebase.ready())
  {
      if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())){
        //Print payload to screen
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        return fbdo.payload();
      }
      else
          Serial.println(fbdo.errorReason());
  }
  return "";
}

FirebaseConnection::FirebaseConnection() {
  //Set up firebase
  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}
