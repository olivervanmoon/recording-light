// hardware setup
const int outputPin = 13; // 13 is also the built-in LED :)

// MIDI stuff
const int MIDIBAUD = 31250; // MIDI spec for baud rate
const int maxMessageLength = 3; // only need messages of length 3 bytes
byte message[maxMessageLength]; // array that stores messages 

// turn light on and off
void setLight(bool recording){
  if (recording){
    digitalWrite(outputPin, HIGH);
  }
  else if (not recording){
    digitalWrite(outputPin, LOW);
  }
}

bool recordingFlagMackie = false; // flag for Mackie Control recording message

bool checkMackieControl(byte message[]){
  /*
  Mackie Control is super chill. Logic Pro X and Ableton Live can use this.
  The DAW sends a single recording on and recording off message on the same note.
  */

  if (message[0] == 0x90 and message[1] == 0x5F){ // note on, note 0x5F 
    if (message[2] == 0x7F){ // on
      recordingFlagMackie = true;
    }
    else if (message[2] == 0x00){ // off
      recordingFlagMackie = false;
    }
  }
  return recordingFlagMackie;
}

// flags for M-Audio Keyboard control surface messages
bool playingFlagMAudio = false;
bool recordEnableFlagMAudio = false;

bool checkMAudioKeyboard(byte message[]){
  /*
  Pro Tools is fucking stupid and doesn't use the Mackie Control protocol, so we're using the
  M-Audio Keyboard control surface setting. This sends out a play and record signal as two separate
  MIDI messages. If both are active, Pro Tools is recording.
  */

  if (message[0] == 0xB0){ // both controls start with this

    if (message[1] == 0x75){ // play on/off
      if (message[2] == 0x7F){
        playingFlagMAudio = true;
      }
      else if (message[2] == 0x00){
        playingFlagMAudio = false;
      }
    }

    if (message[1] == 0x76){ // record on/off
      if (message[2] == 0x7F){
        recordEnableFlagMAudio = true;
      }
      else if (message[2] == 0x00){
        recordEnableFlagMAudio = false;
      }
    }
  }

  return (recordEnableFlagMAudio and playingFlagMAudio);
}  

void setup(){
  pinMode(outputPin, OUTPUT);
  setLight(0);
  Serial.begin(MIDIBAUD);
}

void loop(){
  if ( Serial.available() ){ // wait for message in serial buffer
    if (!(Serial.peek() >> 7 == 1)){ // if it doesn't start with a 1, not the start of a MIDI message
      Serial.read(); // delete byte from serial buffer
    }
    else { // does start with a 1, is the start of MIDI message
      Serial.readBytes(message, maxMessageLength);
      bool recordingStatus = checkMAudioKeyboard(message) or checkMackieControl(message); // could add more if needed
      setLight(recordingStatus);
      }
    }
  }