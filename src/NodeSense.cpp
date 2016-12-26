#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <Esp8266Configuration.h>
#include <DallasTemperature.h>

#define OWBUS 2     // D4 on NodeMCU

OneWire oneWire(OWBUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempSensor;
Esp8266Configuration configuration;

const char* ota_password = "NodeSensePass";

float temperature = 0.0;
char temp_string[15];

void setup_temp_sensors() {
  sensors.begin();
  #ifdef DEBUG
  if (!sensors.getAddress(tempSensor, 0)) Serial.println("Unable to find address for device 0");
  #endif
  sensors.setResolution(9);
}

void setup_ota() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    #ifdef DEBUG
    Serial.println("OTA update initiated");
    #endif
  });
  ArduinoOTA.onEnd([]() {
    #ifdef DEBUG
    Serial.println("\nOTA update completed");
    #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef DEBUG
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    #endif
  });
  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef DEBUG
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    #endif
  });
  ArduinoOTA.begin();

}

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(configuration.getWifiStationSsid(), configuration.getWifiStationPassword());
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #ifdef DEBUG
    Serial.println("Connection Failed! Rebooting...");
    #endif
    delay(5000);
    ESP.restart();
  }
}

void setup_configuration() {
  configuration.read();
  #ifdef DEBUG
  if (configuration.isWifiStationConfigurationValid() == false) {
    Serial.println("Wifi configuration is wrong");
  }
  #endif

}


void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  Serial.println("Booting");
  #endif

  setup_wifi();
  setup_ota();
  setup_temp_sensors();

  #ifdef DEBUG
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("OTA password: ");
  Serial.println(ota_password);
  #endif
}

void loop() {
  ArduinoOTA.handle();

  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  #ifdef DEBUG
  Serial.print("Current temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");
  #endif
  delay(10000);
}
