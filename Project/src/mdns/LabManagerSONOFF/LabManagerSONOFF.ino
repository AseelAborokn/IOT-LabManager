#include "SonoffConfigsManager.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// Assign output variables to GPIO pins
#define LED_GPIO  13
#define RELAY_GPIO  12

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String LED_GPIOState = "off";
String RELAY_GPIOState = "off";

unsigned long currentTime = millis();   // Current time
unsigned long previousTime = 0;         // Previous time
const long timeoutTime = 2000;          // Define timeout time in milliseconds (example: 2000ms = 2s)

void setup() {
  //Set up smart switch as AP to connect to WiFi
  setupSONOFFConfigs();

  // Initialize the output variables as outputs
  pinMode(LED_GPIO, OUTPUT);
  pinMode(RELAY_GPIO, OUTPUT);
  // Set outputs to LOW
  digitalWrite(LED_GPIO, HIGH);
  digitalWrite(RELAY_GPIO, LOW);

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(my_hostname)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("_http", "_tcp", 80);
  MDNS.addServiceTxt("_http", "_tcp", "board", my_hostname);
}

void loop(){
  onDemandConfigSONOFF();
  MDNS.update();
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /led/on") >= 0) {
              Serial.println("Blue LED on");
              LED_GPIOState = "on";
              digitalWrite(LED_GPIO, LOW);
            } else if (header.indexOf("GET /led/off") >= 0) {
              Serial.println("Blue LED off");
              LED_GPIOState = "off";
              digitalWrite(LED_GPIO, HIGH);
            } else if (header.indexOf("GET /relay/on") >= 0) {
              Serial.println("Red LED on");
              RELAY_GPIOState = "on";
              digitalWrite(RELAY_GPIO, HIGH);
            } else if (header.indexOf("GET /relay/off") >= 0) {
              Serial.println("Red LED off");
              RELAY_GPIOState = "off";
              digitalWrite(RELAY_GPIO, LOW);
            } else if (header.indexOf("GET /IP_Address") >= 0) {
              client.print(WiFi.localIP().toString());
            } else {
              client.print("HTTP/1.1 404 Not Found\r\n\r\n");
            }

            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
