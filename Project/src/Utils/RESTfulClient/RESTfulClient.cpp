#include "RESTfulClient.hpp"

// 80 is the default port which the Web-Server will listen to
WebServer server(80);
// jsonDocument to hold the deserialized JSON-object (as JSON format)
StaticJsonDocument<STATIC_JSON_DOCUMENT_SIZE> jsonDocument;
// buffer to hold the serialized JSON-Object (as STRING format)
char buffer[STATIC_JSON_DOCUMENT_SIZE];
// The default way to send/receive data is through JSON format.
char* JSON_APPLICATION_TYPE = "application/json";
// Empty JSON
char* EMPTY_JSON_OBJECT = "{}";

void RESTfulClient::generateErrorPayload(String error) {
    jsonDocument.clear();
    jsonDocument["error"] = error;
    serializeJson(jsonDocument, buffer);
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

    server.send(NOT_FOUND, "text/plain", message);
}

void RESTfulClient::setUpRouteConfigurationsAndBegin() {
    server.onNotFound(handleNotFound);
    server.begin();
}

void RESTfulClient::postHandlerTemplate(char *url, void (*jsonDocumentReader)(JsonDocument)) {
    Serial.print("POST url: ");
    Serial.println(url);
    if (server.hasArg("plain") == false) { }
    String body = server.arg("plain");
    deserializeJson(jsonDocument, body);
    try {
        jsonDocumentReader(jsonDocument);
    }
    catch(const std::exception& e) {
        RESTfulClient::generateErrorPayload(e.what());
        server.send(INTERNAL_ERROR, "application/json", buffer);
    }

    server.send(SUCCESS, "application/json", "{}");
}

void RESTfulClient::getHandlerTemplate(char *url, void (*jsonDocumentAssigner)(JsonDocument*)) {
    Serial.print("GET url: ");
    Serial.println(url);
    try {
        RESTfulClient::createJsonDocument(jsonDocumentAssigner);
    }
    catch(const std::exception& e) {
        RESTfulClient::generateErrorPayload(e.what());
        server.send(INTERNAL_ERROR, "application/json", "{}");
    }
    server.send(SUCCESS, "application/json", buffer);
}

void RESTfulClient::createJsonDocument(void (*jsonDocumentAssigner)(JsonDocument*)) {
    jsonDocument.clear();
    jsonDocumentAssigner(&jsonDocument);
    serializeJson(jsonDocument, buffer);
}

void RESTfulClient::addJsonObject(void (*jsonObjectAssigner)(JsonObject*)) {
    JsonObject obj = jsonDocument.createNestedObject();
    jsonObjectAssigner(&obj);
}

void RESTfulClient::addGetRoute(char* url, void (*jsonDocumentAssigner)(JsonDocument*)) {
    server.on(url, [url, jsonDocumentAssigner](){ RESTfulClient::getHandlerTemplate(url, jsonDocumentAssigner); });
}

void RESTfulClient::addPostRoute(char* url, void (*jsonDocumentReader)(JsonDocument)) {
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