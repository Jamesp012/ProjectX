/*
  SerialLDRTester

  Reads an LDR attached to an analog input pin and prints the current light
  level to the serial port.

  This example code is in the public domain.
*/


#include <DebouncedLDR.h>

constexpr static int LDR_PIN = A0;

// An LDR with readings between [0, 1023] that will return ten light levels
// and will consider readings within 100 of each other to be the same level.
DebouncedLDR ldr(1023, 9, 100);

void setup()
{
    Serial.begin(115200);

    pinMode(LDR_PIN, INPUT);
}

void loop()
{
    static DebouncedLDR::Level prev_level = 0;

    auto level = ldr.update(analogRead(LDR_PIN));

    if (level != prev_level) {
        Serial.print("Light level now ");
        Serial.println(level);
        prev_level = level;
    }

    delay(50);
}

