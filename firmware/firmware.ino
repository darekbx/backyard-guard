/**
 * How to run on mac:
 * 1. ln -s -f /usr/local/bin/python3 /usr/local/bin/python
 * 2. open /Applications/Arduino.app
 */
// Firebase lib: https://github.com/mobizt/Firebase-ESP-Client
#include "credentials.h"
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DallasTemperature.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "OneWire.h"
#include "time.h"

#define DEBUG true
#define LED_ENABLED true
#define THREAD_DELAY 50
#define PIN_LEDATOM 27
#define PIN_PIR 32
#define ONE_WIRE_BUS 21

#if LED_ENABLED
#include <M5Atom.h>
#endif

bool wifiConnected = false;
unsigned long sendDataPrevMillis = 0;
unsigned long tempReadPrevMillis = 0;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {      
  #if LED_ENABLED
    M5.begin(true, false, true);
  #endif
  
  pinMode(PIN_PIR, INPUT);

  #if DEBUG
    Serial.begin(115200);
    while(!Serial); 
  #endif

  // Connect to wifi
  WiFi.begin(ssid, password);

  #if DEBUG
    Serial.print("Connecting");
  #endif

  while (WiFi.status() != WL_CONNECTED) {
    #if LED_ENABLED
      // Blink to indicate that wifi is connecting
      M5.dis.drawpix(0, 0x00FF00);
      M5.update();
      delay(300);
      M5.dis.drawpix(0, 0xFF0000);
      M5.update();

      #if DEBUG
        Serial.print(".");
      #endif
    #endif
  }

  #if DEBUG
    Serial.print("Connected, local IP: ");
    Serial.println(WiFi.localIP());
  #endif

  auth.user.email = firebase_email;
  auth.user.password = firebase_password;
  config.api_key = firebase_web_api_key;
  config.database_url = firebase_database_url;
  config.token_status_callback = tokenStatusCallback;
  config.cert.data = rootCACert;
  config.timeout.serverResponse = 10 * 1000;
  config.timeout.networkReconnect = 10 * 1000;
  config.timeout.rtdbKeepAlive = 45 * 1000;
  config.timeout.rtdbStreamReconnect = 1 * 1000;

  Firebase.reconnectWiFi(true);
  
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth); 
  Firebase.setDoubleDigits(5);

  sensors.begin();

  wifiConnected = true;
}

void loop() {
  if (wifiConnected) {
    if (Firebase.ready() && (millis() - sendDataPrevMillis > 10000  || sendDataPrevMillis == 0)) {
      sendDataPrevMillis = millis();
      #if DEBUG
        Serial.println("Refresh...");
      #endif
      String dbRef = "/data/last-active";
      Firebase.setTimestamp(fbdo, dbRef);
    }

    // Report temperature
    if (millis() - tempReadPrevMillis > 5 * 60 * 1000  || tempReadPrevMillis == 0) {
      tempReadPrevMillis = millis();
      sensors.requestTemperatures();

      float tmp = sensors.getTempCByIndex(0);
      Serial.print("Temperature: ");
      Serial.println(tmp);
      FirebaseJson json;
      json.add("timestamp", getTime());
      json.add("temperature", tmp);

      String dbRef = "/data/temperature";
      Firebase.pushJSON(fbdo, dbRef, json);
      #if DEBUG
        Serial.println("Temp sent");
      #endif
    }

    // Detected movement
    if (digitalRead(PIN_PIR) == 1) {
      #if LED_ENABLED
        // Notify with Blue that movement was occurred
        M5.dis.drawpix(0, 0x0000FF);
        M5.update();
        delay(500);
        M5.dis.drawpix(0, 0x00FF00);
        M5.update();
      #endif
  
      #if DEBUG
        Serial.println("Movement occured!");
      #endif
      
      // TODO send information
      if (Firebase.ready()) {
        String dbRef = "/data/pir-event";
        Firebase.pushTimestamp(fbdo, dbRef);
        #if DEBUG
          Serial.println("Data sent");
        #endif
      } else {
        #if DEBUG
          Serial.println("Firebase not ready!!");
        #endif
      }

      delay(5000);
    }
    
    #if LED_ENABLED
      // Notify with Green that device is connected
      M5.dis.drawpix(0, 0x00FF00);
      M5.update();
    #endif
  } else {
    #if LED_ENABLED
      // Notify with Red color when device is disconnected
      M5.dis.drawpix(0, 0xFF0000);
      M5.update();
    #endif
  }
  
  delay(THREAD_DELAY);
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}
