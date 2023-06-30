int latchPin = 12;
int clockPin = 13;
int dataPin = 14;
int oePin = 5;

String incomingByte;

int dampers[12];

String message;

uint32_t Data;

int positions[12];

float till_100 = 150;

float delaytime = 150 * 100;

int openRelays[12] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22};
int closeRelays[12] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23};

void control(int damperNum, int state)
{
  if (state == 0) {
    converted(openRelays[damperNum]);
    delay(delaytime);
    converted(24);
  } else if (state == 1) {
    converted(closeRelays[damperNum]);
    delay(delaytime);
    converted(24);
  }
}

void converted(int relaynum) {
  Data = 0b000000000000000000000001 << relaynum;
  shifting(Data);
}

void shifting(uint32_t Data)
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, (Data >> 16));
  shiftOut(dataPin, clockPin, MSBFIRST, (Data >> 8));
  shiftOut(dataPin, clockPin, MSBFIRST, Data);
  digitalWrite(latchPin, HIGH);
}

void setup()
{
  Serial.begin(115200);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(oePin, OUTPUT);

  for (int i = 0; i < 12; i++) {
    positions[i] = 1;
    Serial.println("Damper " + String(i) + " position: " + String(positions[i]));
  }

  if (1 == 1) {
    shifting(0b010101010101010101010101);
    delay(delaytime);
    //shifting(0b000000000000000000000000);
  }

  Serial.println("Ready");
}

void loop()
{
  if (Serial.available() > 0) {
    incomingByte = Serial.readStringUntil('\n');
    Serial.print("I received: ");
    Serial.println(incomingByte);
    String relayNumStr = int(incomingByte);
    int curPos = positions[relayNumStr];
    if (curPos == 1) {
      message = message + " Opening";
      positions[relayNumStr] = 0;
    }
    else {
      message = message + " Closing";
      positions[relayNumStr] = 1;
    }
    Serial.println(String(relayNum) + message);
    control(relayNumStr, positions[relayNumStr]);
  }
}
