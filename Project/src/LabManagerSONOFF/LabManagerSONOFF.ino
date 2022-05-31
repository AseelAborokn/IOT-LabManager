#include "SonoffConfigsManager.h"

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String LED_GPIOState = "off";
String RELAY_GPIOState = "off";

// Assign output variables to GPIO pins
#define LED_GPIO  13
#define RELAY_GPIO  12

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  //Set up smart switch as AP to connect to WiFi
  setupSONOFFConfigs();

  // Initialize the output variables as outputs
  pinMode(LED_GPIO, OUTPUT);
  pinMode(RELAY_GPIO, OUTPUT);
  // Set outputs to LOW
  digitalWrite(LED_GPIO, HIGH);
  digitalWrite(RELAY_GPIO, LOW);

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("Setup() - WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  onDemandConfigSONOFF();
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
