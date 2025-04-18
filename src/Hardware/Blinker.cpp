#include "Blinker.h"

TBlinkerManager::TBlinkerManager(int pin, int intervalMS) {
    LED_PIN = pin;
    POLL_INTERVAL = intervalMS;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    showBlinker = true;
}

void TBlinkerManager::Loop() {
    if (showBlinker) {
        if (millis() - lastPoll > POLL_INTERVAL) {
            if (stateLED) {
                digitalWrite(LED_BUILTIN, LOW);
            } else {
                digitalWrite(LED_BUILTIN, HIGH);
            }
            stateLED = !stateLED;
            lastPoll = millis();
        }
    }
}

void TBlinkerManager::Resume() {
    lastPoll = millis();
    showBlinker = true;
}

void TBlinkerManager::Pause() {
    showBlinker = false;
}
