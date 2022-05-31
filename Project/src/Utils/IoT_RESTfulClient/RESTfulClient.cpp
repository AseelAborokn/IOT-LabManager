#include "RESTfulClient.hpp"

// Global variables 
WebServer server(80);                                           // 80 is the default port which the Web-Server will listen to
StaticJsonDocument<STATIC_JSON_DOCUMENT_SIZE> jsonDocument;     // jsonDocument to hold the deserialized JSON-object (as JSON format)
char buffer[STATIC_JSON_DOCUMENT_SIZE];                         // buffer to hold the serialized JSON-Object (as STRING format)

// Data types on HttpRequests / HttpResponses
char* JSON_APPLICATION_TYPE = "application/json";                   // JSON
char* PAIN_TEXT_APPLICATION_TYPE = "text/plain";                    // TEXT
char* URL_APPLICATION_TYPE = "application/x-www-form-urlencoded";   // URL

// Default values
char* EMPTY_JSON_OBJECT = "{}"; // Empty JSON
char* EMPTY_STRING = "";        // Empty STRING




// -------------------------------- IMPLEMENTING PRIVATE Methods --------------------------------

void RESTfulClient::generateErrorPayload(String error) {
  jsonDocument.clear();                 // Clear the memory before inserting new data!
  jsonDocument["error"] = error;        // Adding the Error
  serializeJson(jsonDocument, buffer);  // JSON -> String.
}

void RESTfulClient::handleNotFound() {
  String message = "Route Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  // Sending back 404 error status code with the wanted message format.
  server.send(NOT_FOUND, PAIN_TEXT_APPLICATION_TYPE, message);
}

void RESTfulClient::setUpRouteConfigurationsAndBegin() {
  // Adding handler for unfound routes.
  server.onNotFound(handleNotFound);
  server.begin();
}


void RESTfulClient::createJsonDocument(void (*jsonDocumentAssigner)(JsonDocument_2KB*)) {
  jsonDocument.clear();                   // Clearing old data before inserting new data!
  jsonDocumentAssigner(&jsonDocument);    // Adding new data on the document to be sent.
  serializeJson(jsonDocument, buffer);    // JSON -> String.
}

void RESTfulClient::postHandlerTemplate(char *url, void (*jsonDocumentReader)(JsonDocument_2KB)) {
  Serial.print("POST url: ");
  Serial.println(url);
  if (server.hasArg("plain") == false) { }  // In case we are receiving in another formats
  String body = server.arg("plain");        // Recieve the body of the request as plain text
  
  try {
    // String -> JSON
    DeserializationError error = deserializeJson(jsonDocument, body);
    if(error) {
      Serial.println("postHandlerTemplate - deserializeJson() FAILED!");
      throw(error.c_str());
    }
    // Read the JSON-document
    jsonDocumentReader(jsonDocument);
  }
  catch(const std::exception& e) {
    RESTfulClient::generateErrorPayload(e.what());
    server.send(INTERNAL_ERROR, JSON_APPLICATION_TYPE, buffer);
  }

  // response with success status
  server.send(SUCCESS, JSON_APPLICATION_TYPE, EMPTY_JSON_OBJECT);
}

void RESTfulClient::getHandlerTemplate(char *url, void (*jsonDocumentAssigner)(JsonDocument_2KB*)) {
  Serial.print("GET url: ");
  Serial.println(url);
  try {
    // Init the JSON fields we wat to send as response
    RESTfulClient::createJsonDocument(jsonDocumentAssigner);
  }
  catch(const std::exception& e) {
      RESTfulClient::generateErrorPayload(e.what());
      server.send(INTERNAL_ERROR, JSON_APPLICATION_TYPE, buffer);
  }
  // response with success status
  server.send(SUCCESS, JSON_APPLICATION_TYPE, buffer);
}

void RESTfulClient::addGetRoute(char* url, void (*jsonDocumentAssigner)(JsonDocument_2KB*)) {
  // Create a GET handler for the HttpRequest
  server.on(url, [url, jsonDocumentAssigner](){ RESTfulClient::getHandlerTemplate(url, jsonDocumentAssigner); });
}

void RESTfulClient::addPostRoute(char* url, void (*jsonDocumentReader)(JsonDocument_2KB)) {
  // Create a POST handler for the HttpRequest
  server.on(url, HTTP_POST, [url, jsonDocumentReader](){ RESTfulClient::postHandlerTemplate(url, jsonDocumentReader); });
}

void RESTfulClient::initRESTClient(const char *ssid, const char* password) {
  Serial.begin(115200);
  Serial.print("Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Setup Route Configurations ...");
  RESTfulClient::setUpRouteConfigurationsAndBegin();
}

void RESTfulClient::startListening() {
  server.handleClient();
  delay(2);
}

//-----------------------------------------------------------------------


/**
 * @param String hostName = IP_ADDRESS (192.168.1.106:1880)
 * @param String path = IP_ADDRESS (update-sensor)
 * @param String query = IP_ADDRESS (?temperature=24.37)
 * 
 * => request = http://192.168.1.106:1880/update-sensor?temperature=24.37
 */
const char* RESTfulClient::createRequest(String hostName, String path, String query) {
  return ("http://" + hostName + "/" + path + query).c_str();
}

bool RESTfulClient::httpGetRequest(const char* request, String *payload) {
  HTTPClient http;

  // Connecting to the desired destination
  http.begin(request);

  //Sending the request and recieving the response status
  int httpStatusCode = http.GET();

  // Printing the result of the GET Request
  *payload = http.getString();
  Serial.print("HTTP Response code: ");
  Serial.println(httpStatusCode);
  Serial.print("Payload: ");
  Serial.println(*payload);
  
  // Free located resources
  http.end();
  return (httpStatusCode > 0) ? true : false;
}

bool RESTfulClient::sendGetRequest(String hostName, String path, String query, String *payload) {
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is not Connected!");
    return false;
  }

  // Creating the Request to be sent
  const char *request = RESTfulClient::createRequest(hostName, path, query);
  // Receiving the Payload as string
  if(!RESTfulClient::httpGetRequest(request, payload)) {
    Serial.println("GET Request FAILED!");
    return false;
  }
  else {
    Serial.println("GET Request SUCCEEDED!");
    return true;
  }
}

bool RESTfulClient::urlGET(String hostName, String path, void (*valueParser)(String), String query) {
  // To hold the recieved payload as response
  String payload = EMPTY_STRING;
  
  // Send the request
  if(!RESTfulClient::sendGetRequest(hostName, path, query, &payload)) {
    return false;
  }
  
  try {
    valueParser(payload);
  }
  catch(const std::exception& e) {
    RESTfulClient::generateErrorPayload(e.what());
    Serial.println(e.what());
    return false;
  }

  return true;
}

bool RESTfulClient::jsonGET(String hostName, String path, void (*jsonDocumentReader)(JsonDocument_2KB), String query) {
  // To hold the recieved payload as response
  String payload = EMPTY_JSON_OBJECT;
  
  // Send the request
  if(!RESTfulClient::sendGetRequest(hostName, path, query, &payload)) {
    return false;
  }
  
  try {
    // String -> JSON
    DeserializationError error = deserializeJson(jsonDocument, payload);
    if(error) {
      Serial.println("jsonGET - deserializeJson() FAILED!");
      throw(error.c_str());
    }
    // User handler for the returned data
    jsonDocumentReader(jsonDocument); 
  }
  catch(const std::exception& e) {
    RESTfulClient::generateErrorPayload(e.what());
    Serial.println(e.what());
    return false;
  }

  return true;
}


// -------------------------------- POST HttpRequest Methods --------------------------------

bool RESTfulClient::httpPostRequest(const char* request, String type, String (*getData)()) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, request);
  // Specify content-type header
  http.addHeader("Content-Type", type);

  try {
    // String httpRequestData = "api_key=tPmAT5Ab3j7F9&sensor=BME280&value1=24.25&value2=49.54&value3=1005.14";           
    // Send HTTP POST request & getting the response code
    int httpResponseStatusCode = http.POST(getData());
    // Getting the content of the response body
    String body = http.getString();
  
    // Printing the results of the request
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseStatusCode);
    Serial.print("Response Body: ");
    Serial.println(body);
  
    // Free located resources
    http.end();
    return (httpResponseStatusCode > 0) ? true : false;
  }
  catch(const std::exception& e) {
    Serial.print("POST Request Exception: ");
    Serial.println(e.what());
    return false;
  }
}

bool RESTfulClient::sendPostRequest(String hostName, String path, String type, String (*getData)(), String query) {
  //Check WiFi connection status
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi Disconnected");
    return false;
  }

  // Creating the Request to be sent
  const char *request = RESTfulClient::createRequest(hostName, path, query);
  if(!RESTfulClient::httpPostRequest(request, type, getData)) {
    Serial.println("POST Request FAILED!");
    return false;
  }
  else {
    Serial.println("POST Request SUCCEEDED!");
    return true;
  }
}

bool RESTfulClient::urlPOST(String hostName, String path, String (*getData)(), String query) {
  // Defining the type of the data we are sending
  String type = URL_APPLICATION_TYPE;
  // Send POST request
  return RESTfulClient::sendPostRequest(hostName, path, type, getData, query);
}

bool RESTfulClient::plainTextPOST(String hostName, String path, String (*getData)(), String query) {
  // Defining the type of the data we are sending
  String type = PAIN_TEXT_APPLICATION_TYPE;
  // Send POST request
  return RESTfulClient::sendPostRequest(hostName, path, type, getData, query);
}

bool RESTfulClient::jsonPOST(String hostName, String path, String (*getData)(), String query) {
  // Defining the type of the data we are sending
  String type = JSON_APPLICATION_TYPE;
  // Send POST request
  return RESTfulClient::sendPostRequest(hostName, path, type, getData, query);
}
