#include "Arduino.h"
#include "WiFi.h"
#include "WiFiClient.h"
#include "WiFiManager.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJson.h"
#include "LittleFS.h"
#include "ElegantOTA.h"
#include "WebSerialLite.h"
#include "PubSubClient.h"
#include "config.h"
#include "./Hardware/Blinker.h"
#include "./Hardware/WaterLevel.h"

#define FORCE_CONFIG_MODE_PIN 15
#define WATERLEVEL_PIN 32

TBlinkerManager tblinker(LED_BUILTIN, 1000);
TWaterLevelManager twaterlevel(WATERLEVEL_PIN, 4095, 1000);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

AsyncWebServer server(80);
AsyncEventSource events("/events");

bool shouldSaveConfig = false;
unsigned long lastPoll = 0;

void DebugLog(String text) {
  Serial.println(text); 
  WebSerial.println(text);
}

void connectLittleFS() {
  if (LittleFS.begin(false) || LittleFS.begin(true))
  {
    DebugLog("LittleFS Connected!");
  } else {
    DebugLog("LittleFS FAILED to mount!");
    delay(5000);
    ESP.restart();
  }
}

void saveStaticIPConfig() {
  Serial.println(F("Saving Static IP"));
  JsonDocument json;
  json["STATIC_IP"] = STATIC_IP.toString();
  json["STATIC_SUB"] = STATIC_SUB.toString();
  json["STATIC_GW"] = STATIC_GW.toString();
  json["STATIC_DNS"] = STATIC_DNS.toString();

  File configFile = LittleFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    Serial.println("Failed to open " + String(JSON_CONFIG_FILE) + " file for writing");
  }

  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    Serial.println("Failed to write to file " + String(JSON_CONFIG_FILE));
  }
  configFile.close();
}
bool loadStaticIPConfig() {
  if (!LittleFS.exists(JSON_CONFIG_FILE)) {
    Serial.println(String(JSON_CONFIG_FILE) + " does not exist.");
    return false;
  }
  File configFile = LittleFS.open(JSON_CONFIG_FILE, "r");
  if (!configFile) {
    Serial.println("Failed to open " + String(JSON_CONFIG_FILE));
    return false;
  }
  Serial.println("Reading " + String(JSON_CONFIG_FILE));
  JsonDocument json;
  DeserializationError error = deserializeJson(json, configFile);
  serializeJsonPretty(json, Serial);
  if (error) {
    Serial.println("Unable to deserialize " + String(JSON_CONFIG_FILE));
    return false;
  }
  bool ipIsOk = true;
  ipIsOk &= STATIC_IP.fromString(json["STATIC_IP"].as<String>());
  ipIsOk &= STATIC_SUB.fromString(json["STATIC_SUB"].as<String>());
  ipIsOk &= STATIC_GW.fromString(json["STATIC_GW"].as<String>());
  ipIsOk &= STATIC_DNS.fromString(json["STATIC_DNS"].as<String>());
  return ipIsOk && 
          STATIC_IP != IPAddress(0,0,0,0) && 
          STATIC_SUB != IPAddress(0,0,0,0) && 
          STATIC_GW != IPAddress(0,0,0,0) && 
          STATIC_DNS != IPAddress(0,0,0,0);
}

String convertJSON(float *waterlevel) {
  JsonDocument json;
  json["waterlevel"] = *waterlevel;
  String result;
  serializeJson(json, result);
  return result;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  DebugLog("Entered Conf Mode");

  DebugLog("Config SSID: ");
  DebugLog(myWiFiManager->getConfigPortalSSID());

  DebugLog("Config IP Address: ");
  DebugLog(WiFi.softAPIP().toString());
}

void saveConfigCallback () {
  DebugLog("Should save config");
  shouldSaveConfig = true;
}

void connectWifi(bool forceConfig)
{
  DebugLog("Starting connectWifi");
  // Setup wifi and manager
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setBreakAfterConfig(true);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);

  if (forceConfig || !wm.getWiFiIsSaved()) {
    // Create custom data for configuration
    loadStaticIPConfig();
    WiFiManagerParameter wc_static_ip("STATIC_IP", "IP Address", STATIC_IP.toString().c_str(), 16);       wm.addParameter(&wc_static_ip);
    WiFiManagerParameter wc_static_sub("STATIC_SUB", "Subnet mask", STATIC_SUB.toString().c_str(), 16);   wm.addParameter(&wc_static_sub);
    WiFiManagerParameter wc_static_gw("STATIC_GW", "Gateway", STATIC_GW.toString().c_str(), 16);          wm.addParameter(&wc_static_gw);
    WiFiManagerParameter wc_static_dns("STATIC_DNS", "DNS", STATIC_DNS.toString().c_str(), 16);           wm.addParameter(&wc_static_dns);
    DebugLog("Starting the portal");
    wm.startConfigPortal();
    if (shouldSaveConfig) {
      STATIC_IP.fromString(wc_static_ip.getValue());
      STATIC_SUB.fromString(wc_static_sub.getValue());
      STATIC_GW.fromString(wc_static_gw.getValue());
      STATIC_DNS.fromString(wc_static_dns.getValue());
      saveStaticIPConfig();
    }
    DebugLog("Restarting!");
    delay(2000);
    ESP.restart();
    return;
  }

  DebugLog("Connecting to " + wm.getWiFiSSID());
  if (loadStaticIPConfig()) {
    DebugLog("Configure IP: " + STATIC_IP.toString() + "/" + STATIC_SUB.toString() + " GW:" + STATIC_GW.toString() + " DNS:" + STATIC_DNS.toString());
    WiFi.config(STATIC_IP, STATIC_GW, STATIC_SUB, STATIC_DNS);
  }
  DebugLog("MAC Address: " + WiFi.macAddress());

  bool canConnect = wm.autoConnect();
  if (!canConnect)
  {
    DebugLog("Failed to connect, restarting!");
    delay(5000);
    ESP.restart();
  }
  // Allow to reconnect to WiFi if signal is lost
  WiFi.setAutoReconnect(true);
}

String processDefaults(const String& var) {
  if (var == "FIRMWARE_VERSION") {
    DebugLog(var + "=" + FIRMWARE_VERSION);
    return String(FIRMWARE_VERSION);
  }
  DebugLog(var +"=[Unknown]");
  return String();
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void onReadingReceived() {
  float reading = twaterlevel.GetLastReading();
  String json = convertJSON(&reading);
  DebugLog(json);
  events.send(json.c_str(), "new_readings", millis());
}

void setup() {

  // Forced setup mode
  pinMode(FORCE_CONFIG_MODE_PIN, INPUT_PULLUP);
  // LED
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(32, ANALOG);
  pinMode(33, ANALOG);

  Serial.begin(115200);

  // Wait for serial console to connect
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
  }
  
  connectLittleFS();

  bool forceConfig = false;

  // Check if we're in forced setup mode
  if (digitalRead(FORCE_CONFIG_MODE_PIN) == LOW) {
    forceConfig = true;
  }

  connectWifi(forceConfig);

  // Setup the web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html", false, processDefaults);
  });
  // Setup the API
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"waterlevel\": 82}";
    request->send(200, "application/json", json);
    json = String();
  });
  events.onConnect([](AsyncEventSourceClient *client){
    client->send("Welcome!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  server.serveStatic("/", LittleFS, "/");
  server.onNotFound(notFound);
  
  ElegantOTA.begin(&server);
  WebSerial.begin(&server);
  
  server.begin();

  // Setup sensor
  twaterlevel.onReading(onReadingReceived);

  DebugLog("Started!!");

}

void loop() {
  if (!WiFi.isConnected()) {
      DebugLog("WiFi connection lost ...");
      delay(1000);
    return;
  }
  ElegantOTA.loop();
  tblinker.Loop();
  twaterlevel.Loop();

  // // Check the current time is larger than the POLL_INTERVAL
  // if (millis() - lastPoll > POLL_INTERVAL) {
  //   digitalWrite(LED_BUILTIN, LOW);
  //   lastPoll = millis();
  //   //DebugLog("Current IP is " + WiFi.localIP().toString());
    
  //   int readingOne = analogRead(32);
  //   int readingTwo = analogRead(33);

  //   digitalWrite(LED_BUILTIN, HIGH);
    
  //   // Push the water reading to the browser
  //   waterReading += 5;
  //   if (waterReading > 100) waterReading = 0;
  //   String reading2 = convertJSON(&waterReading);
  //   DebugLog(reading2);
  //   events.send("ping",NULL,millis());
  //   events.send(reading2.c_str(),"new_readings" ,millis());
  // }

}