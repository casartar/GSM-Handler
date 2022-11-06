#include "gsm.h"
#include <Arduino.h>
#include <SoftwareSerial.h>

Gsm gsm(PIN2);

void setup()
{
    gsm.begin(Serial);
    pinMode(A5, INPUT_PULLUP);
    pinMode(A4, OUTPUT);
    digitalWrite(A6, HIGH);
}

void loop()
{
    gsm.handler();
    if (digitalRead(A5) == LOW) {
        digitalWrite(A4, LOW);
        gsm.sendSMS("", "TestAgain");
    } else {
        digitalWrite(A4, HIGH);
    }
}