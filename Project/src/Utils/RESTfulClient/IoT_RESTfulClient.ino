#include "RESTfulClient.hpp"

const char *ssid = "saleh";
const char *password = "0527799259";

int temperature = -1;
//RESTfulClient restClient;

void sayHallo(JsonDocument_2KB* doc) {
    (*doc)["say_hi"] = "hello 3me shokri";
}

void temperatureJsonDocAssigner(JsonDocument_2KB* doc) {
    (*doc)["temperature"] = temperature;
}

void throwsError(JsonDocument_2KB doc) {
    throw std::invalid_argument("AddPositiveIntegers arguments must be positive");
}

void printLolo(JsonDocument_2KB doc) {
    const char *name = doc["name"];
    const char *game = doc["game"];
    int age = doc["age"];

    Serial.println("name -");
    Serial.println(name);
    Serial.println("age -");
    Serial.println(age);
    Serial.println("Game -");
    Serial.println(game);
}

void setup() {
  // put your setup code here, to run once:
  RESTfulClient::addGetRoute("/temperature", temperatureJsonDocAssigner);
  RESTfulClient::addGetRoute("/SayHi", sayHallo);
  RESTfulClient::addPostRoute("/lolo", printLolo);
  RESTfulClient::addPostRoute("/exception", throwsError);
  
  RESTfulClient::initRESTClient(ssid, password);
}

void loop() {
  // put your main code here, to run repeatedly:
  RESTfulClient::startListening();
}
