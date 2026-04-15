#include "protocols.h"

// hardware setup
const int outputPin = 13; // 13 is also the built-in LED :)

// MIDI stuff
const int MIDIBAUD = 31250; // MIDI spec for baud rate
const int maxMessageLength = 3; // only need messages of length 3 bytes
byte message[maxMessageLength]; // array that stores messages

// turn light on and off
void setLight(bool recording) {
  if (recording) { digitalWrite(outputPin, HIGH); }
  else           { digitalWrite(outputPin, LOW);  }
}

void setup() {
  pinMode(outputPin, OUTPUT);
  setLight(false);
  Serial.begin(MIDIBAUD);
  Serial.setTimeout(10);  // don't block >10ms waiting for bytes 2-3 of a message
}

void loop() {
  if (Serial.available()) {
    byte firstByte = Serial.peek();
    if (!(firstByte >> 7)) {
      Serial.read();          // discard non-status byte
    } else if (firstByte == 0xF0) {
      handleSysEx();          // flush SysEx to keep buffer clean
    } else {
      Serial.readBytes(message, maxMessageLength);
      bool recordingStatus = checkHUI(message)
                          or checkMackieControl(message)
                          or checkMAudioKeyboard(message);
      setLight(recordingStatus);
    }
  }
}
