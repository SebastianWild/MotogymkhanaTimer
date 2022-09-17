#include <Arduino.h>
#include <Ultrasonic.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

/*
Core concepts:
- EventSource for sending messages to client
- Button on client & API here to start/stop timing
- Logic for knowing when threshold is crossed:
    - measure delta to previous range
    - if significantly smaller, a threshold has been crossed
    - pause reading for a few seconds
*/

/*
* Constants
*/

// Ranger
#define RANGERPIN 5
float lastTimeInterval = 0.0;
int DELAY = 100;    // msec to delay between readings
const int STEP = 10;  // valid stepping interval in msec
const int MAX_DELAY = 250;    // Max ranging delay
const int MIN_DELAY = 10;   // Minimum ranging delay

// Soft Access Point
const char* ssid = "ESP8266 Timer";
const char* password = "motogymkhana";

/*
* Library Setup
*/

// Ranger
Ultrasonic ultrasonic(RANGERPIN);

// Webserver
AsyncWebServer server(80);

/*
* Main Logic
*/

// index.html contents
const char index_html[] PROGMEM = R"rawLiteral(
<!DOCTYPE HTML><html>
 <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
    </style>
  </head>
  <body>
    <h2>Lap Timer</h2>
    <p>
      <span>Last Trigger</span>
      <span>%TIMEINTERVAL% sec</span>
      <br>
      <br>
      <form action="/update" method="post">
        <label>Ranging Interval (msec)</label>
        <input type="number" id="ranging-interval" name="interval" value="%DELAY%" step="%STEP%" max="%MAX_DELAY%" min="%MIN_DELAY%"><br><br>
        <input type="submit" value="Update">
        <input type="reset" value="Reset">
      </form>
    </p>
  </body>
  <script>

  </script>
</html>
)rawLiteral";

// Simple templating engine processor.
// See: https://github.com/me-no-dev/ESPAsyncWebServer#template-processing
String templateProcessor(const String& var) {
  if (var == "TIMEINTERVAL") {
    return String(lastTimeInterval);
  } else if (var == "DELAY") {
    return String(DELAY);
  } else if (var == "STEP") {
    return String(STEP);
  } else if (var == "MAX_DELAY") {
    return String(MAX_DELAY);
  } else if (var == "MIN_DELAY") {
    return String(MIN_DELAY);
  } else {
    return String();
  }
}

void setup() {
  // Debug
  Serial.begin(9600);

  // Soft Access Point
  WiFi.softAP(ssid, password);    // Default IP 192.168.4.1

  // Webserver route setup
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request -> send_P(200, "text/html", index_html, templateProcessor);
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request -> hasParam("interval", true)) {
      AsyncWebParameter* p = request->getParam("interval", true);
      
      int newdelay = (p -> value()).toInt();
      if (newdelay != 0 && newdelay >= MIN_DELAY) {
        DELAY = (p -> value()).toInt();
        Serial.println("Updated DELAY to: " + String(DELAY));
        request -> redirect("/");
      } else {
        Serial.println("Received invalid new DELAY value: " + p -> value());
        request -> send(401);
      }
    }
  });
  server.begin();
}

void loop() {
  // char _buffer[7];
  // int centimeters;

  // centimeters = ultrasonic.MeasureInCentimeters();
  // delay(DELAY);  // msec

  // sprintf(_buffer, "%03u cm", centimeters);
  // Serial.println(_buffer);  // print distance (in cm) on serial monitor
  delay(3000);
  // Serial.println("-------------------------------------");
  // Serial.println("Number of WiFi clients: " + String(WiFi.softAPgetStationNum()));
  // Serial.println("Last TimeInterval: " + String(lastTimeInterval));
  // Serial.println("DELAY: " + String(DELAY));
  // Serial.println("-------------------------------------");
}