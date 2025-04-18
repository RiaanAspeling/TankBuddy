#ifndef T_HARDWARE_WATERLEVEL_H
#define T_HARDWARE_WATERLEVEL_H

#include "Arduino.h"

class TWaterLevelManager {
    public:
        TWaterLevelManager(int pin, int maxRange, int intervalMs = 1000);
        void Loop();
        float GetReading();
        float GetLastReading() { return lastReading; }
        typedef void (*ReadingReady)();
        void onReading(ReadingReady cb);
    private:
        int pin;
        int maxRange;
        float lastReading = 0;
        int intervalMs;
        unsigned long lastMillis = 0;
        ReadingReady onReadingCallback = nullptr;
};

#endif // T_HARDWARE_WATERLEVEL_H