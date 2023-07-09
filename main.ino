#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "TheBlock";
const char* password = "10cartledge";

ESP8266WebServer server(80);

// HTML page content
const char* htmlPage = "..."; // Your HTML page content goes here

uint32_t relayState = 0; // Bitmask to store the state of relays (32 bits to represent up to 32 relays)

int latchPin = 12;
int clockPin = 13;
int dataPin = 14;
int oePin = 5;

unsigned long relayTimes[12] = {0}; // Stores the target times for turning off the relays

int openRelays[12] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
int closeRelays[12] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23};

void shifting(uint32_t data) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, (data >> 16));
  shiftOut(dataPin, clockPin, MSBFIRST, (data >> 8));
  shiftOut(dataPin, clockPin, MSBFIRST, data);
  digitalWrite(latchPin, HIGH);
}

void updateRelays() {
  shifting(relayState); // Update the relay state on the shift register
}

void setRelayState(int relayNum, int state) {
  if (state == 1) {
    relayState |= (1UL << relayNum); // Set the corresponding bit to 1 to turn on the relay
  } else {
    relayState &= ~(1UL << relayNum); // Set the corresponding bit to 0 to turn off the relay
  }
  updateRelays();
}

void control(int damperNum, int state) {
  int openRelay = openRelays[damperNum - 1];
  int closeRelay = closeRelays[damperNum - 1];
  
  Serial.print("Damper " + String(damperNum));
  if (state == 1) {
    Serial.print(" Opening ");
    setRelayState(openRelay, 1);
    relayTimes[damperNum - 1] = millis() + 15000;
  } else {
    Serial.print(" Closing ");
    setRelayState(closeRelay, 1);
    relayTimes[damperNum - 1] = millis() + 15000;
  }
  Serial.println("Triggering relay " + String(openRelay) + " and " + String(closeRelay));
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleButton() {
  if (server.hasArg("button")) {
    String buttonValue = server.arg("button");
    if (buttonValue.startsWith("on:")) {
      int damper = buttonValue.substring(3).toInt();
      if (damper >= 1 && damper <= 12) {
        control(damper, 1);
      }
    } else if (buttonValue.startsWith("off:")) {
      int damper = buttonValue.substring(4).toInt();
      if (damper >= 1 && damper <= 12) {
        control(damper, 0);
      }
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String status = "[";
  for (int i = 0; i < 12; i++) {
    status += String((relayState >> (closeRelays[i])) & 0x01);
    if (i < 11) {
      status += ",";
    }
  }
  status += "]";
  server.send(200, "application/json", status);
}

void checkRelays() {
  unsigned long currentTime = millis();
  for (int i = 0; i < 12; i++) {
    if (relayTimes[i] > 0 && relayTimes[i] < currentTime) {
      int openRelay = openRelays[i];
      int closeRelay = closeRelays[i];
      
      setRelayState(openRelay, 0);
      setRelayState(closeRelay, 0);
      
      relayTimes[i] = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(oePin, OUTPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/", handleRoot);
  server.on("/button", handleButton);
  server.on("/status", handleStatus);

  server.begin();
  Serial.println("HTTP server started");
}

unsigned long lastLoopTime = 0;

void loop() {
  server.handleClient();

  unsigned long currentTime = millis();
  if (currentTime - lastLoopTime >= 10) {
    lastLoopTime = currentTime;
    checkRelays();
  }
}
