#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

// Define RX and TX pins
#define RX_PIN D7
#define TX_PIN D8

// Create a SoftwareSerial object
SoftwareSerial mySerial(RX_PIN, TX_PIN);

// Wi-Fi credentials and IP configuration
const char* ssid = "YourSSID";
const char* password = "YourPassword";
IPAddress local_IP(192, 168, 2, 184);
IPAddress gateway(192, 168, 2, 254);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

// Create a web server object on port 80
ESP8266WebServer server(80);

unsigned long lastCommandTime = 0;
unsigned int commandIndex = 0;
bool powerOnRequested = false;
bool powerOffRequested = false;

// Function to handle sending power on sequence
void sendPowerOnCommand() {
  if (commandIndex == 0) {
    Serial.println("Powering On...");
  }

  if (commandIndex < 8) {
    if (millis() - lastCommandTime >= 250) {  // 250ms interval for redundant requests
      lastCommandTime = millis();
      mySerial.print("\r*pow=on#\r");
      commandIndex++;
    }
  } else if (commandIndex == 8) {
    if (millis() - lastCommandTime >= 30000) {  // 30 seconds wait
      lastCommandTime = millis();
      Serial.println("Selecting HDMI...");
      commandIndex++;
    }
  } else if (commandIndex < 15) {  // requests for HDMI selection
    if (millis() - lastCommandTime >= 1500) {  // 1.5 second interval
      lastCommandTime = millis();
      mySerial.print("\r*sour=hdmi#\r");
      commandIndex++;
    }
  } else {
    powerOnRequested = false;
    commandIndex = 0;
  }
}

// Function to handle sending power off sequence
void sendPowerOffCommand() {
  if (commandIndex == 0) {
    Serial.println("Powering Off...");
  }

  if (commandIndex < 8) {
    if (millis() - lastCommandTime >= 250) {  // 250ms interval again with backup requests
      lastCommandTime = millis();
      mySerial.print("\r*pow=off#\r");
      commandIndex++;
    }
  } else {
    powerOffRequested = false;
    commandIndex = 0;
  }
}

// Handler for the /on endpoint
void handleOn() {
  powerOnRequested = true;
  powerOffRequested = false;  // Prevents both commands from running simultaneously
  server.send(200, "text/plain", "Power On and HDMI Source Set");
}

// Handler for the /off endpoint
void handleOff() {
  powerOffRequested = true;
  powerOnRequested = false;
  server.send(200, "text/plain", "Power Off");
}

void setup() {
  // Start the built-in Serial
  Serial.begin(115200);

  // Start the software serial port
  mySerial.begin(115200);

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);

  // Set static IP
  if (!WiFi.config(local_IP, gateway, subnet, dns)) {
    Serial.println("IP failed to configure");
  }

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Define the routes
  server.on("/on", handleOn);
  server.on("/off", handleOff);

  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle client requests
  server.handleClient();

  // Check if power on or off command has been requested
  if (powerOnRequested) {
    sendPowerOnCommand();
  } else if (powerOffRequested) {
    sendPowerOffCommand();
  }
}