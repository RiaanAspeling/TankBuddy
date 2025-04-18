#include "WaterLevel.h"

TWaterLevelManager::TWaterLevelManager(int pin, int maxRange, int intervalMs) {
    pinMode(pin, ANALOG);
    this->pin = pin;
    this->maxRange = maxRange;
    this->intervalMs = intervalMs;
    this->lastMillis = millis();
}

void TWaterLevelManager::Loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastMillis >= intervalMs) {
        lastMillis = currentMillis;
        int reading = GetReading();
        if (onReadingCallback) {
            onReadingCallback();
        }
    }
}

float TWaterLevelManager::GetReading() {
    float reading = analogRead(pin);
    reading = map(reading, 0, maxRange, 0, 1000);
    if (reading < 0) reading = 0;
    lastReading = reading / 10.0;
    return lastReading;
}

void TWaterLevelManager::onReading(ReadingReady cb) {
    onReadingCallback = cb;
}