#include "RESTfulClient.hpp"

const char *ssid = "TechPublic";
const char *password = "";

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
    const char *foundKey = doc["foundKey"];
    const char *unknownKey = doc["unknownKey"];

    Serial.println("foundKey -");
    Serial.println(foundKey);
    Serial.println("unknownKey -");
    Serial.println(unknownKey);
}

// In case there was issue with the reader the code should catch it, and return 500 status code with the error occurred
void jsonBuggedReader(JsonDocument_2KB doc) {
    const char *unFoundKey = doc["unFoundKey"];
    const char *accessError = doc["accessError"];
    
    Serial.println("unFoundKey -");
    Serial.println(unFoundKey);
    Serial.println("accessError -");
    Serial.println(accessError);
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
}
