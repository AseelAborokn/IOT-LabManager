#include "RESTfulClient.hpp"

const char *ssid = "LAPTOP-P91HMRUQ-9085";
const char *password = "=12345678";

// This message we want to send as response to GET method
const char* response = "My Response!!!";

// Example on how to assign value to JSON-document
void jsonAssigner(JsonDocument_2KB* doc) {
    (*doc)["response"] = response;
}

// In case there was issue with the assigner the code should catch it, and return 500 status code with the error occurred
void jsonBuggedAssigner(JsonDocument_2KB* doc) {
    throw std::invalid_argument("AddPositiveIntegers arguments must be positive");
}

// Example on how to read JSON
void jsonReader(JsonDocument_2KB doc) {
    char *foundKey, *unknownKey;
    strcpy(foundKey, doc["foundKey"]);
    strcpy(unknownKey, doc["unknownKey"]);
    
    Serial.println("foundKey -");
    Serial.println(foundKey);
    Serial.println("unknownKey -");
    Serial.println(unknownKey);
}

// In case there was issue with the reader the code should catch it, and return 500 status code with the error occurred
void jsonBuggedReader(JsonDocument_2KB doc) {
  
    char *unFoundKey, *accessError;
    strcpy(unFoundKey, doc["unFoundKey"]);
    strcpy(accessError, doc["accessError"]);
    
    Serial.println("unFoundKey -");
    Serial.println(unFoundKey);
    Serial.println("accessError -");
    Serial.println(accessError);
}

void valueParser(String response) {
  Serial.println("valueParser handler, the reponse is -");
  Serial.println(response);
}

void jsonParser(JsonDocument_2KB response) {
  Serial.println("jsonParser handler, the reponse is -");
  char *cod;
  strcpy(cod, response["cod"]);
  int message = response["message"].as<int>();
  int cnt = response["cnt"].as<int>();
  double temp = response["list"][0]["main"]["temp"].as<double>();
  Serial.println(cod);
  Serial.println(message);
  Serial.println(cnt);
  Serial.println(temp);
}

String getDate() {
  Serial.println("getDate handler");
  return "";
}

void setup() {
  // Configure the possible routes which are valid by the ESP32-Web-Server
  RESTfulClient::addGetRoute("/getExample", jsonAssigner);
  RESTfulClient::addGetRoute("/getExampleWithDefaultReader");
  RESTfulClient::addGetRoute("/getThrowsExcepltionExample", jsonBuggedAssigner);
  RESTfulClient::addPostRoute("/postExample", jsonReader);
  RESTfulClient::addPostRoute("/postExampleWithDefaultReader");
  RESTfulClient::addPostRoute("/postThrowsExceptionExample", jsonBuggedReader);

  // After configuring the routes initiate the Web-Server
  RESTfulClient::initRESTClient(ssid, password);
}

void loop() {
  // Start listening on the HttpRequests
  RESTfulClient::startListening();

  String hostName = "api.openweathermap.org";
  String path = "data/2.5/forecast";
  String query = "?id=524901&appid=864c3a42a63a9929ef655b8982950597";
  RESTfulClient::urlGET(hostName, path, valueParser, query);
  delay(5000);
  RESTfulClient::jsonGET(hostName, path, jsonParser, query);
  delay(5000);
  RESTfulClient::urlPOST("reqbin.com", "echo/post/json", getDate);
  delay(5000);
}
