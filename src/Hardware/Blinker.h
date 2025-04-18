#ifndef T_HARDWARE_BLINKER_H
#define T_HARDWARE_BLINKER_H

#include "Arduino.h"

class TBlinkerManager {
    public:
        TBlinkerManager(int pin, int intervalMS = 1000);
        void Loop();
        void Resume();
        void Pause();
    private:
        bool showBlinker = true;
        bool stateLED = false;
        unsigned long lastPoll = 0;
        int LED_PIN;
        long POLL_INTERVAL = 1000;
};

#endif // T_HARDWARE_BLINKER_H