#include "Arduino.h"

#define JSON_CONFIG_FILE "/config.json"
#define FIRMWARE_VERSION 1.00

IPAddress STATIC_IP = IPAddress(0,0,0,0);
IPAddress STATIC_SUB = IPAddress(0,0,0,0);
IPAddress STATIC_GW = IPAddress(0,0,0,0); 
IPAddress STATIC_DNS = IPAddress(0,0,0,0);

unsigned long POLL_INTERVAL = 1000;