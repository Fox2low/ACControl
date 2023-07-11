#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "TheBlock";
const char* password = "10cartledge";

ESP8266WebServer server(80);

// HTML page content
const char* htmlPage =
  "<html>";

int damperStatus[14] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0};

uint32_t relayState = 0;
int latchPin = 12;
int clockPin = 13;
int dataPin = 14;
int oePin = 5;

unsigned long relayTimes[12] = {0};


uint32_t Data;

int positions[12];

float till_100 = 150;

float delaytime = till_100;

int openRelays[12] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
int closeRelays[12] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23};

int Upstairs[4] = {1, 1, 1, 1};
int Downstairs[2] = {1, 1};
int Living[4] = {1, 1, 1, 1};
int Other[4] = {1, 1, 1, 1};

int UpstairsMin = 2;
int DownstairsMin = 1;
int LivingMin = 2;
int OtherMin = 0;

void shifting(uint32_t Data)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, (Data >> 16));
  shiftOut(dataPin, clockPin, MSBFIRST, (Data >> 8));
  shiftOut(dataPin, clockPin, MSBFIRST, Data);
  digitalWrite(latchPin, HIGH);
}

void updateRelays() {
  shifting(relayState);
}

void setRelayState(int relayNum, int state) {
  Serial.print("Relay: ");
  Serial.println(relayNum);
  Serial.print("State: ");
  Serial.println(state);
  if (state == 1) {
    relayState |= (1UL << relayNum);
  } else {
    relayState &= ~(1UL << relayNum);
  }
  updateRelays();
}

void controled(int damperNum, int state) {
  Serial.print("DAMPERNUMBER: ");
  Serial.println(damperNum);

  if (damperNum < 11) {
    int openRelay = openRelays[damperNum - 1];
    int closeRelay = closeRelays[damperNum - 1];

    Serial.print("Damper " + String(damperNum));
    if (state == 1) {
      damperStatus[damperNum - 1] = 1;
      if ((relayState >> openRelay) & 0x01) {
        Serial.println(" Closing open damper " + String(openRelay));
        setRelayState(openRelay, 0);
      }
      if ((relayState >> closeRelay) & 0x01) {
        Serial.println(" Closing close damper " + String(closeRelay));
        setRelayState(closeRelay, 0);
      }

      Serial.print(" Opening ");
      setRelayState(openRelay, 1);
      relayTimes[damperNum - 1] = millis() + 15000;
    } else {
      damperStatus[damperNum - 1] = 0;
      if ((relayState >> closeRelay) & 0x01) {
        Serial.println(" Closing close damper " + String(closeRelay));
        setRelayState(closeRelay, 0);
      }
      if ((relayState >> openRelay) & 0x01) {
        Serial.println(" Closing open damper " + String(openRelay));
        setRelayState(openRelay, 0);
      }

      Serial.print(" Closing ");
      setRelayState(closeRelay, 1);
      relayTimes[damperNum - 1] = millis() + 15000;
    }
    Serial.println("Triggering relay " + String(openRelay) + " and " + String(closeRelay));
  }
  else {
    setRelayState(damperNum + 9, state);
  }
}

bool checkDampersOpen(char group) {
  int* dampers;
  int minOpen;
  int numDampers;

  switch (group) {
    case 'A':
      dampers = Upstairs;
      minOpen = UpstairsMin;
      numDampers = sizeof(Upstairs) / sizeof(Upstairs[0]);
      break;
    case 'B':
      dampers = Downstairs;
      minOpen = DownstairsMin;
      numDampers = sizeof(Downstairs) / sizeof(Downstairs[0]);
      break;
    case 'C':
      dampers = Living;
      minOpen = LivingMin;
      numDampers = sizeof(Living) / sizeof(Living[0]);
      break;
    case 'D':
      return true;
    default:
      return false;
  }

  int numOpen = 0;

  for (int i = 0; i < numDampers; i++) {
    if (dampers[i] == 1) {
      numOpen++;
    }
  }
  //Serial.print("Number Open: ");
  //Serial.println(numOpen);
  //Serial.print("Min Open: ");
  //Serial.println(minOpen);

  if (numOpen >= minOpen + 1) {
    return true;
  } else {
    return false;
  }
}

int relaydamper(int relay) {
  for (int i = 0; i < 12; i++) {
    if (openRelays[i] == relay) {
      return i + 1;
    }
    if (closeRelays[i] == relay) {
      return i + 1;
    }
  }

  Serial.println("Error: Number not found!");
  return 0;
}

char group(int relay) {
  int state;
  int damper;
  damper = relaydamper(relay);

  if (damper >= 1 && damper <= 4) {
    return 'A';
  } else if (damper >= 5 && damper <= 6) {
    return 'B';
  } else if (damper >= 7 && damper <= 10) {
    return 'C';
  } else {
    return 'D';
  }
}

void groupcontrol(int relay, char group) {
  int damper = relaydamper(relay);
  int state;
  if (relay % 2 == 0) {
    state = 1;
    if (group == 'A') {
      Upstairs[damper - 1] = state;
      controled(damper, state);
    } else if (group == 'B') {
      Downstairs[damper - 5] = state;
      controled(damper, state);
    } else if (group == 'C') {
      Living[damper - 7] = state;
      controled(damper, state);
    } else {
      controled(damper, state);
      return;
    }
  }
  else {
    state = 0;
    if (checkDampersOpen(group) == 1) {
      if (group == 'A') {
        Upstairs[damper - 1] = state;
        controled(damper, state);
      } else if (group == 'B') {
        Downstairs[damper - 5] = state;
        controled(damper, state);
      } else if (group == 'C') {
        Living[damper - 7] = state;
        controled(damper, state);
      } else {
        controled(damper, state);
        return;
      }
      return;
    }
    else {
      Serial.println("Too many are closed!");
    }
  }
}

void printcontrol(int relay) {
  Serial.print("Damper Motor: ");
  Serial.print(relaydamper(relay));
  Serial.print(" Which is in group: ");
  Serial.println(group(relay));
  Serial.print("Group has more than 50% open: ");
  Serial.println(checkDampersOpen(group(relay)));
  Serial.println("-------------------");

  Serial.print("[");
  for (int i = 0; i < 4; i++) {
    Serial.print(Upstairs[i]);
  }
  Serial.print("]");

  Serial.print("[");
  for (int i = 0; i < 2; i++) {
    Serial.print(Downstairs[i]);
  }
  Serial.print("]");

  Serial.print("[");
  for (int i = 0; i < 4; i++) {
    Serial.print(Living[i]);
  }
  Serial.println("]");

  Serial.println("-------------------");
}

void control(int relay) {
  int damper = relaydamper(relay);
  char groups = group(relay);
  groupcontrol(relay, groups);
  printcontrol(relay);

}

void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", htmlPage);
}

void handleButton() {
  if (server.hasArg("button")) {
    String buttonValue = server.arg("button");
    if (buttonValue.startsWith("on:")) {
      int damper = buttonValue.substring(3).toInt();
      if (damper >= 1 && damper <= 10) {
        control(openRelays[damper - 1]);
      }
      else {
        damperStatus[damper - 1] = 1;
        controled(damper, 1);
        Serial.print("Special Damper ");
        Serial.print(damper);
        Serial.println(" On");
      }
    } else if (buttonValue.startsWith("off:")) {
      int damper = buttonValue.substring(4).toInt();
      if (damper >= 1 && damper <= 10) {
        control(closeRelays[damper - 1]);
      }
      else {
        damperStatus[damper - 1] = 0;
        controled(damper, 0);
        Serial.print("Special Damper ");
        Serial.print(damper);
        Serial.println(" Off");
      }
    }
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  String status = "[";
  for (int i = 0; i < 14; i++) {
    status += String(damperStatus[i]);
    if (i < 13) {
      status += ",";
    }
  }
  status += "]";
  server.sendHeader("Access-Control-Allow-Origin", "*");
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

void AllOpen() {
  for (int i = 0; i < 11; i++) {
    control(openRelays[i - 1]);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(oePin, OUTPUT);
  AllOpen();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  digitalWrite(LED_BUILTIN, HIGH);
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
