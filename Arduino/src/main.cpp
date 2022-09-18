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

int WINDOW_SIZE = 20;
int DETECTION_SIZE = 5;
int PERCENT_DIFF_TRIGGER = 30;    // If a reading is this % different from before, treat this as a potential trigger
int AFTER_DETECTION_DELAY = 2000;   // Delay after detecting something moving across barrier

RollingWindow window(WINDOW_SIZE);
RollingWindow detection(DETECTION_SIZE);

long currentTimeInterval = 0;   // msec

bool run = false;
long prevTriggerMillis = 0;
bool runDetection = false;

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
      <span id="lasttrigger">%TIMEINTERVAL% sec</span>
      <br>
      <br>
      <button id="btn_startstop" type="button">%STARTSTOP%</button>
      <h3>Settings</h3>
      <form action="/update" method="post">
        <label>Ranging Interval (msec)</label>
        <input type="number" id="ranging-interval" name="interval" value="%DELAY%" step="%STEP%" max="%MAX_DELAY%" min="%MIN_DELAY%">
        <br>
        <label>Window Size</label>
        <input type="number" id="window-size" name="window-size" value="%WINDOW_SIZE%" min="1">
        <br>
        <label>Detection Size</label>
        <input type="number" id="detection-size" name="detection-size" value="%DETECTION_SIZE%" min="1">
        <br>
        <label>Percent Difference Trigger</label>
        <input type="number" id="percent-diff-trigger" name="percent-diff-trigger" value="%PERCENT_DIFF_TRIGGER%" min="1" max="99">
        <br>
        <label>After Detection Delay</label>
        <input type="number" id="after-detection-delay" name="after-detection-delay" value="%AFTER_DETECTION_DELAY%" min="500" max="3000">
        <br>
        <label>Log Level</label>
        <select name="log-level" id="log-level">
          <option value="DEBUG">DEBUG</option>
          <option value="INFO">INFO</option>
          <option value="WARNING">WARNING</option>
          <option value="ERROR">ERROR</option>
        </select>
        <br><br>
        <input type="submit" value="Update">
        <input type="reset" value="Reset">
      </form>
    </p>
    <p>
      <h3>Log</h3>
      <br>
      <textarea readonly id="log" rows="50" cols="50"></textarea>
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

    document.addEventListener('DOMContentLoaded', function () {
      if (!!window.EventSource) {
        var source = new EventSource('/events');

        source.addEventListener('open', function(e) {
          console.log("Events Connected");
        }, false);

        source.addEventListener('error', function(e) {
          if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
          }
        }, false);

        source.addEventListener('log', function(e) {
          document.getElementById('log').value += (e.data + "\n");
        }, false);

        source.addEventListener('trigger', function(e) {
          const msec = e.data
          const sec = (parseFloat(msec) / 1000).toFixed(3);
          document.getElementById('lasttrigger').innerHTML = (sec + " sec");
        }, false);
      }
    }, false);
  </script>
</html>
)rawLiteral";

enum LogLevel: int {
  DEBUG = 0,
  INFO = 1,
  WARNING = 2,
  ERROR = 3
};

int percentDifference(double lhs, double rhs) {
  return abs(lhs - rhs) / ((lhs + rhs) / 2) * 100;
}

/// @brief Send trigger time over EventSource connection
/// @param triggerMillis Trigger time in msec to convert to string and send
void sendTrigger(long triggerMillis) {
  events.send(String(triggerMillis).c_str(), "trigger", millis());
}

LogLevel logLevel = INFO;
/// @brief Send log message over EventSource connection
/// @param level The log level
/// @param message The message
void sendLog(LogLevel level, String message) {
  Serial.println(message);
  if (logLevel >= level)
    events.send(message.c_str(), "log", millis());
}

void restart() {
  sendLog(DEBUG, "Resetting");

  runDetection = false;
  detection.clear();
  window.clear();
  prevTriggerMillis = 0;
  currentTimeInterval = 0;
}

/// Simple templating engine processor.
/// See: https://github.com/me-no-dev/ESPAsyncWebServer#template-processing
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
  } else if (var == "WINDOW_SIZE") {
    return String(WINDOW_SIZE);
  } else if (var == "DETECTION_SIZE") {
    return String(DETECTION_SIZE);
  } else if (var == "PERCENT_DIFF_TRIGGER") {
    return String(PERCENT_DIFF_TRIGGER);
  } else if (var == "AFTER_DETECTION_DELAY") {
    return String(AFTER_DETECTION_DELAY);
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
        sendLog(INFO, "Updated DELAY to: " + String(DELAY));
        request -> redirect("/");
      } else {
        sendLog(WARNING, "Received invalid new DELAY value: " + p -> value());
        request -> send(401);
      }
    }

    if (request -> hasParam("start", true)) {
      run = !run;
      request -> send(200);
      restart();
    }

    if (request -> hasParam("window-size", true)) {
      AsyncWebParameter* p = request->getParam("window-size", true);
      int windowSize = (p -> value()).toInt();
      if (windowSize < 1) {
        request -> send(400);
        sendLog(WARNING, "Received invalid new WINDOW_SIZE value: " + p -> value());
        return;
      }
      sendLog(INFO, "Updated WINDOW_SIZE to: " + String(windowSize));

      WINDOW_SIZE = windowSize;
      restart();
    }

    if (request -> hasParam("detection-size", true)) {
      AsyncWebParameter* p = request->getParam("detection-size", true);
      int windowSize = (p -> value()).toInt();
      if (windowSize < 1) {
        request -> send(400);
        sendLog(WARNING, "Received invalid new DETECTION_SIZE value: " + p -> value());
        return;
      }
      sendLog(INFO, "DETECTION_SIZE WINDOW_SIZE to: " + String(windowSize));

      DETECTION_SIZE = windowSize;
      restart();
    }

    if (request -> hasParam("percent-diff-trigger", true)) {
      AsyncWebParameter* p = request->getParam("percent-diff-trigger", true);
      int param = (p -> value()).toInt();
      if (param < 1 || param > 99) {
        request -> send(400);
        sendLog(WARNING, "Received invalid new PERCENT_DIFF_TRIGGER value: " + p -> value());
        return;
      }
      sendLog(INFO, "PERCENT_DIFF_TRIGGER to: " + String(param));
      
      PERCENT_DIFF_TRIGGER = param;
      restart();
    }

    if (request -> hasParam("after-detection-delay", true)) {
      AsyncWebParameter* p = request->getParam("after-detection-delay", true);
      int param = (p -> value()).toInt();
      if (param < 500 || param > 3000) {
        request -> send(400);
        sendLog(WARNING, "Received invalid new AFTER_DETECTION_DELAY value: " + p -> value());
        return;
      }
      sendLog(INFO, "AFTER_DETECTION_DELAY to: " + String(param));

      AFTER_DETECTION_DELAY = param;
      restart();
    }

    if (request -> hasParam("log-level", true)) {
      AsyncWebParameter* p = request->getParam("log-level", true);
      String level = p -> value();
      // Could probably just have frontend pass int here...
      if (level == "DEBUG") {
        logLevel = DEBUG;
      } else if (level == "INFO") {
        logLevel = INFO;
      } else if (level == "WARNING") {
        logLevel = WARNING;
      } else if (level == "ERROR") {
        logLevel = ERROR;
      } else {
        sendLog(WARNING, "Received invalid new log level: " + level);
        return;
      }
      sendLog(INFO, "Set new log level: " + level);
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

  int reading = ultrasonic.MeasureInCentimeters();
  window.append(reading);
  int average = window.average();

  // we must allow window to fill in order for our average calculation to be useful
  if (window.size() != WINDOW_SIZE) {
    sendLog(DEBUG, "Calibrating - Window not yet filled.");
    delay(DELAY);
    return;
  }

  const int percentDiff = percentDifference(reading, average);
  
  if (runDetection == false && percentDiff >= PERCENT_DIFF_TRIGGER) {
    runDetection = true;
    sendLog(INFO, "Reading % difference:" + String(percentDiff) + " Starting detection.");
  }

  if (runDetection && detection.size() == 0) {
    // Initial measurement. Start rolling window to see if we actually have a trigger
    runDetection = true;
    prevTriggerMillis = timeSinceBoot;
    detection.append(reading);
    
    sendLog(DEBUG, "Potential trigger. Starting stopwatch. Starting detection phase.");
  } else if (runDetection && detection.size() < DETECTION_SIZE) {
    // Still need to fill detection window...
    detection.append(reading);
    
    sendLog(DEBUG, String("Detection ") + String(detection.size()) + String("/") + String(DETECTION_SIZE));
  } else if (runDetection && detection.size() >= DETECTION_SIZE) {
    // Window is full...Check if conditions are right for a trigger
    sendLog(DEBUG, "Detection window full.");
    runDetection = false;
    if (percentDifference(detection.average(), window.average()) > PERCENT_DIFF_TRIGGER) {
      // TRIGGER
      currentTimeInterval = millis() - prevTriggerMillis;
      sendTrigger(currentTimeInterval);
      sendLog(INFO, String("Trigger! ") + String("Detection average: ") + String(detection.average()) + String(" Window average: ") + String(window.average()));
      prevTriggerMillis = 0;

      delay(AFTER_DETECTION_DELAY);
    } // we have a fluke, ignore
  }
  
  delay(DELAY);

  // Serial.println("-------------------------------------");
  // Serial.println("Number of WiFi clients: " + String(WiFi.softAPgetStationNum()));
  // Serial.println("Last TimeInterval: " + String(currentTimeInterval));
  // Serial.println("DELAY: " + String(DELAY));
  // Serial.println("Window average:" + String(window -> average()));
  // Serial.println("-------------------------------------");
}
