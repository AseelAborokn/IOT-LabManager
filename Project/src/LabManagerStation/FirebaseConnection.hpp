#include <Firebase_ESP_Client.h>

#ifndef _FIREBASECONNECTION_H_
#define _FIREBASECONNECTION_H_

class FirebaseConnection {
  private:
    // The Firestore payload upload callback function
    void fcsUploadCallback(CFS_UploadStatusInfo info);
    /*
     * Add a field to FirebaseJson content
     * @param content - FirebaseJson to add field to
     * @param fieldName - The name of the field as seen under the document
     * @param fieldType - Type of field for fieldName, passed in small letters in this manner:
     *  in case of String fieldType == "string"
     *  in case of Time Stamp fieldType == "timestamp"
     *  in case of int fieldType == "integer"
     *  in case of bool fieldType == "boolean"
     * @param fieldValue - The value to be added to content under fieldName
     */
    static void addField(FirebaseJson *content, String fieldName, String fieldType, String fieldValue);
    /*
     * Add an integer field to FirebaseJson content
     * @param content - FirebaseJson to add field to
     * @param fieldName - The name of the field as seen under the document
     * @param fieldValue - The value to be added to content under fieldName
     */
    static void addField(FirebaseJson *content, String fieldName, int fieldValue);
    /*
     * Add a field filter to query
     * This works when adding one filter or multiple filters to a query
     * We only use filtering for equality, for other uses change "EQUAL" under op in code according to structured query rules
     * @param query - FirebaseJson that holds a structured query to run
     * @param index - non-negative value to indicate the position of the filtered field we are adding to query
     * @param fieldName - The name of the field as seen under the document
     * @param fieldValue - The value to be added to content under fieldName
     */
    static void addFilter(FirebaseJson *query, int index, String fieldName, String fieldValue);
    /*
     * Get field value
     * @param documentPath - relative path to document, including document name
     * @param fieldType - Type of field for fieldName
     * @param fieldName - The name of the field as seen under the document
     * @return - in success, a String of the value under fieldName in document.
     *           in failure, empty String 
     */
    static String getField(String documentPath, String fieldType, String fieldName);
    /*
     * Get a field from first object in query
     * Should be called after runQuery is called
     * @param payload - The payload result from runQuery as String
     * @param fieldPath - fieldPath as seen in payload (should start as "document/...")
     * if payload includes more than one query fails
     * @return - in success, a specific field from query in payload
     *           in failure, empty string
     */
    static String getQueryField(String payload, String fieldPath);
    /*
     * Upadte a field in a document
     * @param documentPath - relative path to document, including document name
     * @param fieldType - Type of field for fieldName
     * @param fieldName - The name of the field as seen under the document
     * @param newData - The value to insert in fieldName
     */
    static void updateDocumentField(String documentPath, String fieldName, String fieldType, String newData);
  public:
    /*
     * Get Field from payload, used when wanting to svae multiple API requests to get fields from one document
     * @param payload - The payload that includes the document
     * @param fieldType - Type of field for fieldName
     * @param fieldName - The name of the field as seen under the documentfield
     * @return Value of fieldName in payload 
     */
    static String getFieldByPayload(String payload, String fieldType, String fieldName);
    /*
     * Get the name of the document as seen in collection
     * Can be called after creating document or updating document
     * @param payload - The payload result from creating document or updating it as String
     * @param pathToName - relative path to name field (should be collection ID)
     * @return - the name of the document created
     */
    static String getDocumentName(String payload, String pathToName);
    /*
     * Update Usage History document 
     * @param doc_name - The name of the document as seen in collection 
     * @param fieldType - Type of field for fieldName
     * @param fieldName - The name of the field as seen under the document
     * @param newData - The value to insert in fieldName
     */
    static void updateUsageHistoryRecordField(String doc_name, String fieldName, String fieldType, String newData);
    /*
     * Get a station field value under fieldName
     * @param doc_name - The name of the document as seen in collection
     * @param fieldType - Type of field for fieldName
     * @param fieldName - The name of the field as seen under the document
     * @return - Field Value under fieldName in doc_name as a String
     */
    static String getStationField(String doc_name, String fieldType, String fieldName);
    /*
     * Get a UsageHistory field value under fieldName
     * @param doc_name - The name of the document as seen in collection
     * @param fieldType - Type of field for fieldName
     * @param fieldName - The name of the field as seen under the document
     * @return - Field Value under fieldName in doc_name as a String
     */
    static String getUseDocField(String doc_name, String fieldType, String fieldName);
    /*
     * Add a station to Firestore
     * @param doc_name - The name of the station document to be created, can pass empty string to auto generate document ID
     * @param accessibility - Document field, "public" or "restricted" to indicate station usage to public
     * @param station_name - Document field
     * @param owner_id - Document field, indicated owner of the station as seen under LabUsers collection in document name (ID)
     * @param run_time_in_secs - Document field
     * @param station_status - Document field, "available" or "unavailable" to indicate station status
     * @return - The name of the document created as seen under collection LabStations
     */
    static String addStationToFirebase(String doc_name, String accessibility, String station_name, String owner_id, int run_time_in_secs, String station_status);
    /*
     * Add a Usage History document to Firestore
     * @param station_name - Document field, station id as seen in document name (ID) under LabStations collection
     * @param user_id - Document field, indicated user of the station as seen under LabUsers collection in document name (ID)
     * @param start_time - Document field, indicates start usage time should be called with value returned from printLocalTime() as char*
     * @param use_time - Document field, seconds of use time, should be called with value of Station's run_time_in_secs
     * @return - The name of the document created as seen under collection UsageHistory
     */
    static String recordUsageInHistory(String station_name, String user_id, char* start_time, int use_time);
    /*
     * Update Finished_at field in document 
     * For our purposes, finished_at field is changed when station user scans for extension
     * Add ussage time to current time to update field
     * @param doc_name - he name of the document to be updated, as seen in Usage History
     * @param use_time - seconds of use time to be added, should be called with value of Station's run_time_in_secs
     */
    //TODO:: When adapting to station add global run_time instead of use_time
    static void extendUsageRecord(String doc_name, int use_time);
    /*
     * Runs query by IDs to get Permission Status
     * @param station_id - The ID of the station being used, as seen in the document name of LabStation
     * @param owner_id - The ID of the station owner, as seen in the document name of LabUsers
     * @param user_id - The ID of the card scanner, trying to use the station, as seen in the document name of LabUsers
     * @return - the value under permission_status in the document queried
     * Query should follow a structure according to https://firebase.google.com/docs/firestore/reference/rest/v1/StructuredQuery
     */
    static String getPermissionStatus(String station_id, String owner_id, String user_id);
    /*
     * Runs query by unique field
     * @param cid - card ID
     * @return - user ID that cid belongs to, as seen by document name in LabUsers
     */
    static String getUserID(String cid);
    /*
     * Runs query by unique fields
     * @param station_name - Name of station as seen in station document
     * @param owner_id - Id of owner as seen in station document
     * @return - in success, Station document as String
     *           in failure, empty String
     * Can use getQueryField to get station parameters from returned value, path should include "dcoument/fields/fieldName/fieldType"
     */
     static String getStationDoc(String station_id);
    /*
     * Initalize Firebase 
     * Should be called in setup
     * Time can be setup before or after this method but should be done before using the other methods
     */
    FirebaseConnection();
};
#endif
