#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "main.h"
#include "credentials.h"

volatile bool pirTrigger = false;
volatile bool flameTrigger = false;
volatile long pirLastUpdate = 0;
volatile long flameLastUpdate = 0;

uint8_t LED = D8;
uint8_t PIR_SENSOR_PIN = D6;
uint8_t FLAME_SENSOR_PIN = D7;

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);


#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
void setup() {
    Serial.begin(115200);
    Serial.println("");
    Serial.println(F("Init"));
    Serial.println(F("Reset reason: "));
    Serial.println(ESP.getResetReason());

    pinMode(PIR_SENSOR_PIN, INPUT_PULLUP);
    Serial.println(F("PIR_SENSOR_PIN set to INPUT_PULLUP"));

    pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP);
    Serial.println(F("FLAME_SENSOR_PIN set to INPUT"));

    pinMode(LED, OUTPUT);
    Serial.println(F("LED set to OUTPUT"));

    Serial.print(F("Attaching interrupt to: "));
    Serial.println(PIR_SENSOR_PIN);
    attachInterrupt(PIR_SENSOR_PIN, pirInterrupt, RISING);

    Serial.print(F("Attaching interrupt to: "));
    Serial.println(FLAME_SENSOR_PIN);
    attachInterrupt(FLAME_SENSOR_PIN, flameInterrupt, FALLING);


    Serial.println(F("Ready."));

    pinMode(LED, OUTPUT);
    connectWifi();
    client.setServer(MQTT_SERVER, 8883);
}
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
void loop() {
    if (pirTrigger) {
        digitalWrite(LED, HIGH);
        reconnect();
        boolean success = client.publish("athens/wemos2/presence", "true");
        Serial.print(F("Published: "));
        Serial.println(success);
        pirTrigger = false;
    }
    if (flameTrigger) {
        digitalWrite(LED, HIGH);
        Serial.printf("Flame triggered. Waiting 3 seconds to check again. Current value %d\r\n",
                      analogRead(FLAME_SENSOR_PIN));
        delay(3000);
        if (digitalRead(FLAME_SENSOR_PIN) == 0) {
            reconnect();
            boolean success = client.publish("athens/wemos2/flame", "true");
            Serial.print(F("Published: "));
            Serial.println(success);
        }
        flameTrigger = false;
    }
    if (pirLastUpdate + 5000 < millis()) {
        digitalWrite(LED, LOW);
    } else if (flameLastUpdate + 5000 < millis()) {
        digitalWrite(LED, LOW);
    }
    Serial.printf("PIR: %d, FLM: %d\r\n", digitalRead(PIR_SENSOR_PIN), digitalRead(FLAME_SENSOR_PIN));
    delay(100);
}
#pragma clang diagnostic pop

void connectWifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print(F("Connecting to "));
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println(F(""));
    Serial.println(F("WiFi connected"));
    Serial.println(F("IP address: "));
    Serial.println(WiFi.localIP());
}

void reconnect() {
    while (!client.connected()) {
        Serial.print(F("Attempting MQTT connection..."));
        // Attempt to connect
        if (client.connect("wemos2", MQTT_USER, MQTT_PASS)) {
            Serial.println(F("connected"));
        } else {
            Serial.print(F("failed, rc="));
            Serial.print(client.state());
            Serial.println(F(" try again in 5 seconds"));
            delay(5000);
        }
    }
}


void pirInterrupt() {
    pirTrigger = true;
    pirLastUpdate = millis();
}

void flameInterrupt() {
    flameTrigger = true;
    flameLastUpdate = millis();
}

