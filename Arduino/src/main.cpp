#include <Arduino.h>
#include <Ultrasonic.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include "RollingWindow.h"

/*
Core concepts:
- EventSource for sending messages to client
- Button on client & API here to start/stop timing
- Logic for knowing when threshold is crossed:
    - Establish baseline distance
    - pause reading for a few seconds
*/

/*
* Constants
*/

// Ranger
#define RANGERPIN 5

RollingWindow window(20);

long currentTimeInterval = 0;   // msec

bool run = false;

int DELAY = 100;    // msec to delay between readings
const int STEP = 10;  // valid stepping interval in msec
const int MAX_DELAY = 500;    // Max ranging delay
const int MIN_DELAY = 50;   // Minimum ranging delay

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
AsyncEventSource events("/events");

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
      <button id="btn_startstop" type="button">%STARTSTOP%</button>
      <form action="/update" method="post">
        <label>Ranging Interval (msec)</label>
        <input type="number" id="ranging-interval" name="interval" value="%DELAY%" step="%STEP%" max="%MAX_DELAY%" min="%MIN_DELAY%"><br><br>
        <input type="submit" value="Update">
        <input type="reset" value="Reset">
      </form>
    </p>
  </body>
  <script>
    document.getElementById("btn_startstop").addEventListener("click", () => {
        let xhr = new XMLHttpRequest(), data = "start=toggle";
        xhr.open('POST', '/update')
        xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xhr.send(data)

        xhr.addEventListener("load", (event) => {
          window.location.reload(false)
        })
    });
  </script>
</html>
)rawLiteral";

// Simple templating engine processor.
// See: https://github.com/me-no-dev/ESPAsyncWebServer#template-processing
String templateProcessor(const String& var) {
  if (var == "TIMEINTERVAL") {
    return String(currentTimeInterval);
  } else if (var == "DELAY") {
    return String(DELAY);
  } else if (var == "STEP") {
    return String(STEP);
  } else if (var == "MAX_DELAY") {
    return String(MAX_DELAY);
  } else if (var == "MIN_DELAY") {
    return String(MIN_DELAY);
  } else if (var == "STARTSTOP") {
    return run ? "Stop" : "Start";
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

    if (request -> hasParam("start", true)) {
      run = !run;
      request -> send(200);
    }
  });
  
  // EventSource setup
  events.onConnect([](AsyncEventSourceClient *client) {
    client -> send(String(currentTimeInterval).c_str(), NULL, millis(), 1000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {
  if (!run) {
    delay(DELAY);
    return;
  }
  
  long timeSinceBoot = millis();

  int centimeters = ultrasonic.MeasureInCentimeters();
  window.append(centimeters);

  
  delay(DELAY);

  // Serial.println("-------------------------------------");
  // Serial.println("Number of WiFi clients: " + String(WiFi.softAPgetStationNum()));
  // Serial.println("Last TimeInterval: " + String(currentTimeInterval));
  // Serial.println("DELAY: " + String(DELAY));
  // Serial.println("Window average:" + String(window -> average()));
  // Serial.println("-------------------------------------");
}
