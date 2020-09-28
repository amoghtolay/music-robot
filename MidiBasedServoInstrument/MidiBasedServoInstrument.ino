/*
 * Arduino Due based code for controlling servos which are connected to musical robots (servos).
 * These are controlled by MIDI over USB on the Arduino Due's native USB port.
 * NOTE: This is a Work-In-Progress.
 * 
 * @author Amogh
 */
#include <MIDIUSB.h>
#include <Servo.h>

/*
 * TODO: Issues
 * 1. Servo shakes/jitters at destination - Reduced the range of motion now.
 * 2. Currently, the servo aims to move to the max endPos but because noteOff is called, so, it stops moving and returns back.
 * Hence, the current behavior is that the motor moves for as long as the note is pressed.
 * 3. 
 */


struct Instrument {
  const byte pin;
  
  Instrument(byte attachPin) : pin(attachPin){}
  void setup() {
    pinMode(pin, OUTPUT);
  }
  virtual void noteOn(byte pitch, byte velocity);
  virtual void noteOff(byte pitch, byte velocity);
};

struct EggShaker : Instrument {
  uint8_t startPos;
  uint8_t endPos;
  Servo shakerServo;
  EggShaker(byte attachPin, uint8_t startPos, uint8_t endPos) : Instrument(attachPin), startPos(startPos), endPos(endPos) {
      shakerServo.attach(attachPin);
  }
  void noteOn(byte pitch, byte velocity) {
      Serial.print("Note On: ");
      Serial.print(pitch);
      Serial.print(", velocity=");
      Serial.println(velocity);
      shakerServo.write(startPos);      
  }

  void noteOff(byte pitch, byte velocity) {
      Serial.print("Note Off: ");
      Serial.print(pitch);
      Serial.print(", velocity=");
      Serial.println(velocity);
      shakerServo.write(endPos);
  }
};

void controlChange(byte channel, byte control, byte value) {
  Serial.print("Control change: control=");
  Serial.print(control);
  Serial.print(", value=");
  Serial.print(value);
  Serial.print(", channel=");
  Serial.println(channel);
}

void printDebugMessages(midiEventPacket_t midiEvent) {
  Serial.print("Raw MIDI message: ");
  Serial.print(midiEvent.header, HEX);
  Serial.print("-");
  Serial.print(midiEvent.byte1, HEX);
  Serial.print("-");
  Serial.print(midiEvent.byte2, HEX);
  Serial.print("-");
  Serial.println(midiEvent.byte3, HEX);

  if (midiEvent.header == 0xB) {
    controlChange(
      midiEvent.byte1 & 0xF,  //channel
      midiEvent.byte2,        //control
      midiEvent.byte3         //value
    );
  }
}

Instrument *instruments[] = {
  new EggShaker(2, 30, 100) // Attach on Pin 2
};

const uint8_t numInstruments = sizeof (instruments) / sizeof (instruments[0]);

void setup() {
  for (uint8_t i = 0; i < numInstruments; i++) {
    instruments[i]->setup();
  }
  Serial.begin(115200);
}

void loop() {
  midiEventPacket_t rx = MidiUSB.read();
  uint8_t channel;
  
  switch (rx.header) {
    case 0:
      break; //No pending events
      
    case 0x9: // Note On Message
      channel = rx.byte1 & 0xF;
      if (channel >= numInstruments) {
        Serial.print("Channel not connected");
        printDebugMessages(rx);
        break;
      }
      instruments[channel]->noteOn(rx.byte2, rx.byte3); // 2 = pitch, 3 = velocity
      break;
      
    case 0x8: // Note Off Message
      channel = rx.byte1 & 0xF;
      if (channel >= numInstruments) {
        Serial.print("Channel not connected");
        printDebugMessages(rx);
        break;
      }
      instruments[channel]->noteOff(rx.byte2, rx.byte3); // 2 = pitch, 3 = velocity
      break;
      
    default:
      printDebugMessages(rx);
      break;
  }
}
