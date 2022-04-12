#include <Arduino.h>
#include <WiFi.h>           // For connecting the ESP32 to our local-WiFi
#include <WebServer.h>      // For setting up the ESP32 as WebServer
#include <ArduinoJson.h>    // Arduino JSON library
#include <FreeRTOS.h>
#include <string>           //
#include <exception>        // For handling exceptions
#include <stdexcept>        //

#define STATIC_JSON_DOCUMENT_SIZE 2048 // 2KB size is all the needed

/**
 *  WiFi HTTP statuses
 */
enum HTTP_STATUS {
    SUCCESS = 200,
    NOT_FOUND = 404,
    INTERNAL_ERROR = 500,
    TIME_OUT = 504
};

typedef StaticJsonDocument<STATIC_JSON_DOCUMENT_SIZE> JsonDocument_2KB;

class RESTfulClient {
private:
    /**
     *  This default method doesn't read anything from the JSON-document.
     */
    static void defaultJsonDocumnetReader(JsonDocument_2KB) {}
    /**
     * This default method doesn't assign anything to the JSON-document.
     */
    static void defaultJsonDocumnetAssigner(JsonDocument_2KB*) {}
    /**
     * This method generates a Json-Object-payload with the following format
     * { "error" - error }
     * @param error - The error occurred in executing the handlers
     */
    static void generateErrorPayload(String error);

    /**
     * This method returns a HttpRequest handler (Status: NOT_FOUND) for
     * the case in which the route is not found!
     *
     * It send HttpResponse in the following format (Plain Text):
     *
     * URI: <requested url>
     * Method: <POST / GET>
     * Arguments: <number of arguments in the url route>
     *  <arg_1_key>: <arg_1_value>
     *  <arg_2_key>: <arg_2_value>
     *  ...
     *  plain: <the JSON object which was sent as payload>
     */
    static void handleNotFound();

    /**
     * This method configs the added routes to the server
     */
    static void setUpRouteConfigurationsAndBegin();

    /**
     * This methods creates a POST HttpRequest handler template (sends HttpResponse for called request).
     * @param url - the desired url-route.
     * @param jsonDocumentReader - pointer to a function that reads the received JSON-document.
     */
    static void postHandlerTemplate(char *url, void (*jsonDocumentReader)(JsonDocument_2KB));

    /**
     * This methods creates a GET HttpRequest handler template (sends HttpResponse for called request).
     * @param url - the desired url-route.
     * @param jsonDocumentAssigner - pointer to a function that inserts the data to a clean JSON-document.
     */
    static void getHandlerTemplate(char *url, void (*jsonDocumentAssigner)(JsonDocument_2KB*));

    /**
      * This method creates a new JSON-document.
      * @param jsonDocumentAssigner  - pointer to a function that inserts the data to a clean JSON-document.
      */
    static void createJsonDocument(void (*jsonDocumentAssigner)(JsonDocument_2KB*));

public:
    /**
      * This Method initiates the RESTfulClient template
      * - MUST BE CALLED AT setup() method!
      * - MUST ADD ALL THE ROUTES BEFORE CALLING THIS METHOD (see below)!
      *
      * @param ssid - the WiFi ssid
      * @param password - the WiFi password
      */
    static void initRESTClient(const char *ssid, const char* password);

    /**
     * This Method start listening to the clients HttpRequests/HttpResponses on port 80
     * - URL route: http://<IP_ADDRESS>/<Configured_Route>
     * - MUST BE CALLED AT THE loop() METHOD!
     */
    static void startListening();

    /**
    * This methods creates a GET HttpRequest/HttpResponse handler
    * @param url - the desired url-route.
    * @param jsonDocumentAssigner - pointer to a function that inserts the data to a clean JSON-document.
    */
    static void addGetRoute(char* url, void (*jsonDocumentAssigner)(JsonDocument_2KB*) = RESTfulClient::defaultJsonDocumnetAssigner);

    /**
     * This methods creates a POST HttpRequest/HttpResponse handler.
     * @param url - the desired url-route.
     * @param jsonDocumentReader - pointer to a function that reads the received JSON-document.
     */
    static void addPostRoute(char* url, void (*jsonDocumentReader)(JsonDocument_2KB) = RESTfulClient::defaultJsonDocumnetReader);

    /**
     * This method creates a new JSON-Object and adds it to the JSON-document.
     * @param jsonObjectAssigner - pointer to a function which assigns data to the new JSON-object
     */
    static void addJsonObject(void (*jsonObjectAssigner)(JsonObject*));
};
