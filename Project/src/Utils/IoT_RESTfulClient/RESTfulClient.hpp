#include <Arduino.h>
#include <HTTPClient.h>     // For sending POST & GET HttpRequest from the ESP32 to our WebApplication
#include <WiFi.h>           // For connecting the ESP32 to our local-WiFi
#include <WebServer.h>      // For setting up the ESP32 as WebServer
#include <ArduinoJson.h>    // Arduino JSON library
#include <string>           // For handling Strings.
#include <exception>        // For handling exceptions
#include <stdexcept>        // For catching global exception

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

/**
 * RESTful Client abstarction class - 
 * This class acts as both WebServer and RESTfulClient, you can add UrlRoutes to listen to and send HttpRequest/recieve HttpResponses
 * When using it to send HttpRequests please make sure to handle the JSON-Objects & JSON-Documents by yourself (Memory usages).
 */
class RESTfulClient {
  private:
    /**
     *  This default method doesn't read anything from the JSON-document.
     */
    static void defaultJsonDocumnetReader(JsonDocument_2KB) {}

    /**
     *  This default method doesn't read anything from the string.
     */
    static void defaultValueReader(String) {}

    /**
     *  This default method returns empty string.
     */
    static String defaultGetStringData() {return "";}

    /**
     *  This default method returns empty string.
     */
    static String defaultGetJsonData() {return "{}";}
    
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

    // -------------------------------- HTTP REQUEST METHODS --------------------------------

    /**
     *
     * @param hostName - The host IP_ADDRESS / SSID (192.168.1.106:1880)
     * @param path  - The url route (update-sensor)
     * @param query - The query to add to the route (?temperature=24.37)
     * @return HttpRequest url (http://192.168.1.106:1880/update-sensor?temperature=24.37)
     */
    static const char* createRequest(String hostName, String path, String query);

    /**
     *  This method tries to send a GET request (helper function).
     * @param request - HttpRequest URL
     * @param payload - Pointer to string in order to store the returned response data.
     * @return TRUE if the HttpRequest succeeds and FALSE otherwise.
     */
    static bool httpGetRequest(const char* request, String *payload);

    /**
     *  This method tries to send a GET request.
     * @param hostName - The host IP_ADDRESS / SSID (192.168.1.106:1880)
     * @param path - The url route (update-sensor)
     * @param query - The query to add to the route (?temperature=24.37)
     * @param payload - Pointer to string in order to store the returned response data.
     * @return TRUE if the HttpRequest succeeds and FALSE otherwise.
     */
    static bool sendGetRequest(String hostName, String path, String query, String *payload);

    /**
     *  This method tries to send a POST request (helper function).
     * @param request - HttpRequest URL
     * @param type - The type of data to be sent (JSON / TEXT / URL)
     * @param getData - Pointer to function which returns the wanted data as string.
     * @return
     */
    static bool httpPostRequest(const char* request, String type, String (*getData)());

    /**
     *  This method tries to send a POST request.
     * @param hostName - The host IP_ADDRESS / SSID (192.168.1.106:1880) 
     * @param path - The url route (update-sensor)
     * @param type - The type of data to be sent (JSON / TEXT / URL)
     * @param getData - Pointer to function which returns the wanted data as string.
     * @param query  - The query to add to the route (?temperature=24.37)
     * @return 
     */
    static bool sendPostRequest(String hostName, String path, String type, String (*getData)(), String query);
    
  public:

    // -------------------------------- WEB SERVICE METHODS --------------------------------
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
    
    // -------------------------------- GET HttpRequest Methods --------------------------------
    
    /**
     *  This method tries to send a GET request (With response data in plain url format).
     * @param hostName - The host IP_ADDRESS / SSID (192.168.1.106:1880)
     * @param path - The url route (update-sensor)
     * @param query - The query to add to the route (?temperature=24.37)
     * @param valueParser - Function that gets the response data as plain text to be parsed by the user.
     * @return TRUE if the HttpRequest succeeds and FALSE otherwise.
     */
    static bool urlGET(String hostName, String path, void (*valueParser)(String) = RESTfulClient::defaultValueReader, String query = "");

    /**
     *  This method tries to send a GET request (With response data in JSON format).
     * @param hostName - The host IP_ADDRESS / SSID (192.168.1.106:1880)
     * @param path - The url route (update-sensor)
     * @param query - The query to add to the route (?temperature=24.37)
     * @param jsonParser - Function that gets the response data as JSON to be parsed by the user.
     * @return TRUE if the HttpRequest succeeds and FALSE otherwise.
     */
    static bool jsonGET(String hostName, String path, void (*jsonDocumentReader)(JsonDocument_2KB) = RESTfulClient::defaultJsonDocumnetReader, String query = "");

    // -------------------------------- POST HttpRequest Methods --------------------------------
    
    /**
     * This method tries to send a POST request (With data in the url).
     * @param hostName - The host IP_ADDRESS / SSID (192.168.1.106:1880)
     * @param path - The url route (update-sensor)
     * @param getData
     * @param query - The query to add to the route (?temperature=24.37)
     * @return TRUE if the HttpRequest succeeds and FALSE otherwise.
     */
    static bool urlPOST(String hostName, String path, String (*getData)() = RESTfulClient::defaultGetStringData, String query = "");

    /**
     * This method tries to send a POST request (With data as plain text).
     * @param hostName - The host IP_ADDRESS / SSID (192.168.1.106:1880)
     * @param path - The url route (update-sensor)
     * @param message - The message to be sent.
     * @param query - The query to add to the route (?temperature=24.37)
     * @return TRUE if the HttpRequest succeeds and FALSE otherwise.
     */
    static bool plainTextPOST(String hostName, String path, String (*getData)() = RESTfulClient::defaultGetStringData, String query = "");

    /**
     * This method tries to send a POST request (With data as JSON).
     * @param hostName - The host IP_ADDRESS / SSID (192.168.1.106:1880)
     * @param path - The url route (update-sensor)
     * @param getData
     * @param query - The query to add to the route (?temperature=24.37)
     * @return TRUE if the HttpRequest succeeds and FALSE otherwise.
     */
    static bool jsonPOST(String hostName, String path, String (*getData)() = RESTfulClient::defaultGetJsonData, String query = "");
};
