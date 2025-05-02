/*
  Description: To test turn ON - OFF relay via local MQTT broker. The sketch is designed to test the ESP32 board.
*/

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <TickTwo.h>

//******************************** Configulation ****************************//
#define _DEBUG_  // Comment this line to disable debug messages

// #define activeHighRelay  // Uncomment this line to use activeHighRelay
#ifdef activeHighRelay
#define turnOn  HIGH  // Active high
#define turnOff LOW
#else
#define turnOn  LOW  // Active low
#define turnOff HIGH
#endif

//******************************** Variables & Objects **********************//
#define ssid       "REPLACE_WITH_YOUR_SSID"
#define password   "REPLACE_WITH_YOUR_PASSWORD"
#define mqttBroker "REPLACE_WITH_YOUR_MQTT_BROKER_IP_ADDRESS"
#define deviceName "REPLACE_WITH_YOUR_DEVICE_NAME"
#define mqttUser   "REPLACE_WITH_YOUR_MQTT_USER"
#define mqttPass   "REPLACE_WITH_YOUR_MQTT_PASSWORD"

#define subInputState "server/input/relayState"

#define relayPin 2
uint8_t relayState;

Preferences pf;

WiFiClient   espClient;
PubSubClient mqtt(espClient);

//******************************** Tasks ************************************//
void    connectMqtt();
void    reconnectMqtt();
TickTwo tConnectMqtt(connectMqtt, 0, 0, MILLIS);  // (function, interval, iteration, interval unit)
TickTwo tReconnectMqtt(reconnectMqtt, 3000, 0, MILLIS);

//********************************  Functions *******************************//
void connectMqtt() {
    if (!mqtt.connected()) {
        tConnectMqtt.stop();
        tReconnectMqtt.start();
    } else {
        mqtt.loop();
    }
}

void reconnectMqtt() {
#ifdef _DEBUG_
    Serial.print(F("Connecting MQTT... "));
#endif
    if (mqtt.connect(deviceName, mqttUser, mqttPass)) {
        tReconnectMqtt.stop();
#ifdef _DEBUG_
        Serial.println(F("connected"));
#endif
        tConnectMqtt.start();
        mqtt.subscribe(subInputState);
    } else {
#ifdef _DEBUG_
        Serial.print(F("failed state: "));
        Serial.println(mqtt.state());
#endif
        if (tReconnectMqtt.counter() > 36) ESP.restart();  // Restart after 3 minutes (5 sec * 36 time = 3 minutes)
    }
}

void setupWifi() {
#ifdef _DEBUG_
    Serial.println();
    Serial.print(F("Connecting to "));
    Serial.println(ssid);
#endif

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
#ifdef _DEBUG_
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
#endif
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
#ifdef _DEBUG_
    Serial.print(F("Message arrived on topic: "));
    Serial.print(topic);
    Serial.print(F(". Message: "));
#endif
    String msg;

    for (int i = 0; i < length; i++) {
        Serial.print((char)message[i]);
        msg += (char)message[i];
    }

    if (!strcmp(topic, subInputState)) {
        relayState = msg.toInt();
        pf.putUChar("kR", relayState);
#ifdef _DEBUG_
        Serial.print(F(" State: "));
        Serial.println(relayState ? "ON" : "OFF");
#endif
    }
}

//********************************  Setup ***********************************//
void setup() {
#ifdef _DEBUG_
    Serial.begin(115200);
#endif
    pf.begin("memory", false);
    relayState = pf.getUChar("kR", 0);
    pinMode(relayPin, OUTPUT);
    setupWifi();
    mqtt.setCallback(mqttCallback);
    mqtt.setServer(mqttBroker, 1883);

    tConnectMqtt.start();
}

//********************************  Loop ************************************//
void loop() {
    tConnectMqtt.update();
    tReconnectMqtt.update();

    digitalWrite(relayPin, relayState ? turnOn : turnOff);
}