#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <DHT.h>

#include "settings.h"

DHT dht(12, DHT22);

long start;

void setup() {
  start = millis();
  Serial.begin(9600);

  Serial.println();
  Serial.println("Setup");

  // Disable AP mode
  WiFi.enableAP(false);
  
#if 1
  // Static IP saves ~600ms
  WiFi.config(IP, GATEWAY, IPAddress(255, 255, 255, 0));
#endif
  WiFi.begin(SSID, PASSPHRASE);
  dht.begin();
}

static bool readData(float * temperature, float * humidity) {
  for (int i = 0; i < 3; i++) {
    *temperature = dht.readTemperature(false, true);
    *humidity = dht.readHumidity();
    if (!isnan(*temperature) && !isnan(*humidity)) {
      return true;
    }
  }
  return false;
}

static bool readMedianData(float * temperature, float * humidity) {
  float t1, t2, t3;
  float h1, h2, h3;
  if (!readData(&t1, &h1) || !readData(&t2, &h2) || ! readData(&t3, &h3)) {
    return false;
  }

  // Return the median of the three
  *temperature = _max(_min(t1, t2), _min(_max(t1 ,t2), t3));
  *humidity = _max(_min(h1, h2), _min(_max(h1 ,h2), h3));
  return true;
}

static void sendData(const float temperature, const float humidity) {
  HTTPClient http;
  http.begin(URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("key=" KEY "&temperature=" + String(temperature) + "&humidity=" + String(humidity));

  if (httpCode > 0) {
    if (httpCode != HTTP_CODE_OK) {
      Serial.println("HTTP POST failed code: " + String(httpCode));
    }
  } else {
    Serial.println("HTTP POST failed error: " + http.errorToString(httpCode));
  }
  http.end();
}

void loop() {
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
#if 0
    Serial.println("Duration: " + String(millis() - start) + " ms");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.gatewayIP());
    Serial.println(WiFi.subnetMask());
#endif
    float temperature;
    float humidity;
    if (!readMedianData(&temperature, &humidity)) {
      Serial.println("Failed to read DHT sensor");
    } else {
#if 0
      Serial.println("temperature: " + String(temperature));
      Serial.println("humidity: " + String(humidity));
#endif
      sendData(temperature, humidity);
    }
    Serial.println("Took: " + String(millis() - start) + " ms");
  } else {
    Serial.println("Failed to connect");
  }
  ESP.deepSleep(1 * 60 * 1000 * 1000); // 1 min
}

