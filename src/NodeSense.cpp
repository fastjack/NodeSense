#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <Esp8266Configuration.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#define OWBUS 2     // D4 on NodeMCU
#define WIFI_RETRY_UPDATE 200 // update every 200 ms
#define WIFI_RETRY_LIMIT 30*5 // 30 seconds
#define STATUS_LED LED_BUILTIN
#define TEMPERATURE_TOPIC "home/livingroom/temperature"

OneWire oneWire(OWBUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempSensor;
Esp8266Configuration configuration;
WiFiClient espClient;
PubSubClient client(espClient);

const char* ota_password = "NodeSensePass";

float temperature = 0.0;
char temp_string[15];

void setup_temp_sensors() {
  sensors.begin();
  #ifdef DEBUG
  if (!sensors.getAddress(tempSensor, 0)) Serial.println("Unable to find address for device 0");
  #endif
  sensors.setResolution(12);
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
const char spinner[4] = {'/', '-', '\\', '|'};
static int retry_counter = 0;
char hostname[32];

  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  sprintf(hostname, "NodeSense_%06X", ESP.getChipId());
  WiFi.hostname((char *)hostname);
  WiFi.mode(WIFI_STA);
  #ifdef DEBUG
  Serial.print("Connecting to WiFi:  ");
  #endif
  WiFi.begin(configuration.getWifiStationSsid(), configuration.getWifiStationPassword());
  while (WiFi.status() != WL_CONNECTED && retry_counter < WIFI_RETRY_LIMIT) {
    #ifdef DEBUG
    Serial.printf("\b%c", spinner[retry_counter%4]);
    #endif
    digitalWrite(STATUS_LED, retry_counter%2 == 0 ? HIGH : LOW);
    retry_counter++;
    delay(WIFI_RETRY_UPDATE);
  }
  if (retry_counter >= WIFI_RETRY_LIMIT) {
    #ifdef DEBUG
    Serial.println("\nTimeout reached. Could not connect to WiFi. Rebooting...");
    #endif
    ESP.restart();
  }
  digitalWrite(STATUS_LED, HIGH);
}

void setup_configuration() {
  configuration.read();
  #ifdef DEBUG
  if (configuration.isWifiStationConfigurationValid() == false) {
    Serial.println("Wifi configuration is wrong");
  }
  #endif
}

void setup_mqtt() {
  if (configuration.isMqttConfigurationValid() && configuration.isMqttEnabled()) {
    client.setServer(configuration.getMqttServer(), configuration.getMqttPort());
  }
}

void reconnect_mqtt() {
  if (!client.connected()) {
    #ifdef DEBUG
    Serial.println("Trying to connect to MQTT broker...");
    #endif
    if (client.connect(configuration.getMqttDeviceName(), configuration.getMqttUser(), configuration.getMqttPassword())) {
      #ifdef DEBUG
      Serial.println("Connection established");
      #endif
    } else {
      #ifdef DEBUG
      Serial.print("Connection failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      Serial.print("using server: ");
      Serial.print(configuration.getMqttServer());
      Serial.print(":");
      Serial.println(configuration.getMqttPort());
      #endif
    }
  }
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  Serial.println("Booting");
  #endif
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);

  setup_configuration();
  setup_wifi();
  setup_ota();
  setup_temp_sensors();
  setup_mqtt();

  #ifdef DEBUG
  Serial.println("\nReady");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("OTA password: ");
  Serial.println(ota_password);
  #endif
}

void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect_mqtt();
  }

  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  dtostrf(temperature, 2, 2, temp_string);
  #ifdef DEBUG
  Serial.print("Current temperature: ");
  Serial.print(temp_string);
  Serial.println(" °C");
  #endif
  client.publish(TEMPERATURE_TOPIC, temp_string, true);
  delay(10000);
}
