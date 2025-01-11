#include "Arduino.h"
#include "WiFi.h"
#include "WiFiClient.h"
#include "WiFiManager.h"
#include "ESPAsyncWebServer.h"
#include "ArduinoJSON.h"
#include "LittleFS.h"
#include "ElegantOTA.h"
#include "WebSerialLite.h"
#include "PubSubClient.h"
#include "config.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

AsyncWebServer server(80);

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
  DynamicJsonDocument json(2048);
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
  DynamicJsonDocument json(2048);
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

bool loadConfig() {
  bool isStatic = loadStaticIPConfig();
  return true;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
  DebugLog("Entered Conf Mode");

  DebugLog("Config SSID: ");
  DebugLog(myWiFiManager->getConfigPortalSSID());

  DebugLog("Config IP Address: ");
  DebugLog(WiFi.softAPIP().toString());
}

void connectWifi(bool forceConfig)
{
    // Setup wifi and manager
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setBreakAfterConfig(true);
  // wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);

  // // Create custom data for configuration
  // WiFiManagerParameter wc_victron_address("VICTRON_HOST", "Victron MQTT Address/Host", VICTRON_HOST, 50);   wm.addParameter(&wc_victron_address);
  // IntParameter wc_victron_port("VICTRON_PORT", "Victron MQTT Port", VICTRON_PORT);                          wm.addParameter(&wc_victron_port);
  // StringParameter wc_victron_id("VICTRON_ID", "Victron Id", VICTRON_ID);                                    wm.addParameter(&wc_victron_id);

  if (forceConfig) {
    wm.startConfigPortal();
    Serial.println("Restarting and resettings Static IP!");
    STATIC_IP = IPAddress(0,0,0,0);
    STATIC_SUB = IPAddress(0,0,0,0);
    STATIC_GW = IPAddress(0,0,0,0);
    STATIC_DNS = IPAddress(0,0,0,0);
    saveStaticIPConfig();
    delay(2000);
    ESP.restart();
    return;
  }

  if (wm.getWiFiIsSaved()) {
    Serial.println("Connecting to " + wm.getWiFiSSID());
    if (loadStaticIPConfig()) {
      Serial.println("Configure IP: " + STATIC_IP.toString() + "/" + STATIC_SUB.toString() + " GW:" + STATIC_GW.toString() + " DNS:" + STATIC_DNS.toString());
      WiFi.config(STATIC_IP, STATIC_GW, STATIC_SUB, STATIC_DNS);
    }
  }
  else
  {
    Serial.println("Connect to configure:");
    Serial.println(wm.getDefaultAPName());
  }
  bool canConnect = wm.autoConnect();
  
  if (!canConnect)
  {
    Serial.println("Failed to connect, restarting!");
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

void setup() {

  // Forced setup mode
  pinMode(15, INPUT_PULLUP);
  // LED
  pinMode(LED_BUILTIN, OUTPUT);

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

  bool loadedConfig = loadConfig();

  // Check if we're in forced setup mode
  if (digitalRead(15) == LOW) {
    loadedConfig = false;
  }

  connectWifi(!loadedConfig);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html", false, processDefaults);
  });
  server.serveStatic("/", LittleFS, "/");
  server.onNotFound(notFound);
  
  ElegantOTA.begin(&server);
  WebSerial.begin(&server);
  
  server.begin();

  DebugLog("Started!!");

}

void loop() {
  if (!WiFi.isConnected()) {
      DebugLog("WiFi connection lost ...");
      delay(1000);
    return;
  }
  ElegantOTA.loop();

  // Check the current time is larger than the POLL_INTERVAL
  if (millis() - lastPoll > POLL_INTERVAL) {
    digitalWrite(LED_BUILTIN, LOW);
    lastPoll = millis();
    DebugLog("Current IP is " + WiFi.localIP().toString());
    digitalWrite(LED_BUILTIN, HIGH);
  }

}