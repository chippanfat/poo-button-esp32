#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

const char* ssid     = SECRET_SSID;
const char* wifiPassword = SECRET_PASS;
const int mqttPort = MQTT_PORT;
const char *mqttBroker = MQTT_BROKER;
const char *mqttUsername = MQTT_USERNAME;
const char *mqttPassword = MQTT_PASSWORD;
const char *mqttTopic = MQTT_TOPIC;

const int buttonPin = 2;
const int buzzerPin = 3;
const int successLedPin = 4;
const int connectionLedPin = 5;
const int failedConnectionLedPin = 6;

unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 10000;

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

// Load DigiCert Global Root CA ca_cert, which is used by EMQX Cloud Serverless Deployment
const char* ca_cert= \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"\
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"\
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"\
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"\
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"\
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"\
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"\
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"\
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"\
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"\
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"\
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"\
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"\
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"\
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"\
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"\
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"\
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"\
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"\
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"\
"-----END CERTIFICATE-----\n";

void setup()
{
    pinMode(buttonPin, INPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(successLedPin, OUTPUT);
    pinMode(connectionLedPin, OUTPUT);
    pinMode(failedConnectionLedPin, OUTPUT);

    digitalWrite(successLedPin, LOW);
    digitalWrite(connectionLedPin, LOW);

    Serial.begin(115200);
    WiFi.begin(ssid, wifiPassword);

    // Wait for the WiFi event
    while (true) {
        
        switch(WiFi.status()) {
          case WL_NO_SSID_AVAIL:
            Serial.println("[WiFi] SSID not found");
            break;
          case WL_CONNECT_FAILED:
            Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
            return;
            break;
          case WL_CONNECTION_LOST:
            Serial.println("[WiFi] Connection was lost");
            break;
          case WL_SCAN_COMPLETED:
            Serial.println("[WiFi] Scan is completed");
            break;
          case WL_DISCONNECTED:
            Serial.println("[WiFi] WiFi is disconnected");
            break;
          case WL_CONNECTED:
            Serial.println("[WiFi] WiFi is connected!");
            Serial.print("[WiFi] IP address: ");

            Serial.println(WiFi.localIP());
            wifiClient.setCACert(ca_cert);

            client.setServer(mqttBroker, mqttPort);

            if (!client.connected()) {
              String clientId = "esp32-client-";
              clientId += String(WiFi.macAddress());
              if (client.connect(clientId.c_str(), mqttUsername, mqttPassword)) {
                Serial.println("[info] - MQTT broker connected");
                Serial.print("[info] - Subscribing to topic - ");
                Serial.print(MQTT_TOPIC);
                Serial.println();
                client.subscribe(mqttTopic);
                digitalWrite(connectionLedPin, HIGH);
                digitalWrite(failedConnectionLedPin, LOW);

              } else {
                Serial.println("[error] - Unable to connect to MQTT broker");
                Serial.println(client.state());
                Serial.println("[info] - Retrying...");
                digitalWrite(failedConnectionLedPin, HIGH);
                digitalWrite(connectionLedPin, LOW);
                break;
              }
            }
            return;
          default:
            Serial.print("[WiFi] WiFi Status: ");
            Serial.println(WiFi.status());
            break;
        }
        delay(500);
    }
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "esp32-client-";
    clientId += String(WiFi.macAddress());
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword)) {
      Serial.println("[info] - MQTT broker connected");
      Serial.print("[info] - Subscribing to topic - ");
      Serial.print(mqttTopic);
      Serial.println();
      client.subscribe(mqttTopic);
      digitalWrite(connectionLedPin, HIGH);
      digitalWrite(failedConnectionLedPin, LOW);
    } else {
      Serial.println("[error] - Unable to connect to MQTT broker");
      Serial.println(client.state());
      Serial.println("[info] - Retrying...");
      digitalWrite(connectionLedPin, LOW);
      digitalWrite(failedConnectionLedPin, HIGH);
      digitalWrite(successLedPin, LOW);
    }
  }
}

void soundBuzzer() {
  Serial.println("[info] - Sound buzzer");
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
  delay(200);
  digitalWrite(buzzerPin, HIGH);
  delay(200);
  digitalWrite(buzzerPin, LOW);
}

void publishMessage() {
  const int result = client.publish(MQTT_TOPIC, "{\"button\": \"true\"}");
  Serial.print("[info] - MQTT response ");
  Serial.print(result);
  Serial.println("");
  
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }

  if (digitalRead(buttonPin) == HIGH && (millis() - lastDebounce) > debounceDelay) {
    lastDebounce = millis();
    soundBuzzer();
    publishMessage();
  }

  if ((millis() - lastDebounce) > debounceDelay) {
    digitalWrite(successLedPin, HIGH);
  } else {
    digitalWrite(successLedPin, LOW);
  }

  client.loop();
}
