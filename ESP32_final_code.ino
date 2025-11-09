    #include <WebServer.h>
    #include "EmonLib.h"  // Include EmonLib library for energy monitoring
    #include <HTTPClient.h>
    #include <EEPROM.h>
    #include <ArduinoJson.h>
    #include<WiFi.h>
    #include <WiFiClientSecure.h>

    #define EEPROM_SIZE 128
    #define SENSOR_PIN 25  // ADC pin where SCT-013 is connected
    #define BURDEN_RESISTOR 100  // Burden resistor value (Ω)
    #define CALIBRATION_FACTOR 29.0

    EnergyMonitor emon;  
    const float FIXED_VOLTAGE = 230.0;  // Fixed voltage value (Volts)
    const float RUN_TIME_HOURS = 3.0; 
   WebServer server(80);// Create a web server on port 80
    String receivedSSID, receivedPassword, userId;
    const char* supabaseUrl = "https://bzlqfggvuwehqsyfctbf.supabase.co/rest/v1/dryers";
    const char* supabaseKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImJ6bHFmZ2d2dXdlaHFzeWZjdGJmIiwicm9sZSI6ImFub24iLCJpYXQiOjE3Mzg2NzY5OTQsImV4cCI6MjA1NDI1Mjk5NH0.qEMRW44NzPEYwqlNIx4K82xy7mMjbd6KIxmVNgoMIlg";


    // Define GPIO pins for each device
    const int fanPin = 14;
    const int lightPin = 15;
    const int heaterPin = 16;
    const int UvPin = 17;
    const int m1 = 23;
    const int m2 = 22;



    // Variables to keep track of device states
    bool fanState = false;
    bool lightState = false;
    bool heaterState = false;
    bool UvState = false;
    bool motorForward = false;
    bool motorbackward =  false;


    void setup() {
      Serial.begin(115200);
      EEPROM.begin(EEPROM_SIZE);
      loadCredentials();

      if (receivedSSID.length() > 0 && receivedPassword.length() > 0) {
    Serial.println("Trying to connect with saved credentials...");
    WiFi.begin(receivedSSID.c_str(), receivedPassword.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
     if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi! IP: " + WiFi.localIP().toString());
      sendToSupabase();
      return;
    } else {
      Serial.println("\nFailed to connect with saved credentials.");
    }
  }
      
    emon.current(SENSOR_PIN, CALIBRATION_FACTOR);
      // Set up GPIO pins as outputs
      pinMode(fanPin, OUTPUT);
      pinMode(lightPin, OUTPUT);
      pinMode(heaterPin, OUTPUT);
      pinMode(UvPin, OUTPUT);
      pinMode( m1, OUTPUT);
      pinMode(m2,OUTPUT);
      // Set initial states to LOW (OFF)
      digitalWrite(fanPin, LOW);
      digitalWrite(lightPin, LOW);
      digitalWrite(heaterPin, LOW);
      digitalWrite(UvPin, LOW);
      digitalWrite(m1, LOW);
      digitalWrite(m2, LOW);


       WiFi.softAP("ESP32-Setup", "12345678");
      Serial.println("Access Point Started. Connect and POST to /config");

      server.on("/config", HTTP_POST, handleConfig);
      server.begin();

      // Set up URL routes and their handlers
      server.on("/", handleRoot);  // Root URL
      server.on("/toggleFan", handleFanToggle);     // URL to toggle fan
      server.on("/toggleLight", handleLightToggle); // URL to toggle light
      server.on("/toggleHeater", handleHeaterToggle); // URL to toggle heater
      server.on("/toggleUv", handleUvToggle);
      server.on("/toggleM1",handleMforward);
      server.on("/toggleM2",handleMbackward);

      // Start the server
      server.begin();
      Serial.println("HTTP server started");
    }

    void loop() {
       server.handleClient();
      double current = emon.calcIrms(1480);  // Calculate RMS current (samples = 1480)
        double power = current * FIXED_VOLTAGE;  // Calculate power (P = V × I)
        double energy = power * RUN_TIME_HOURS;  // Calculate energy consumption in watt-hours

        // Print values
        Serial.print("Current: ");
        Serial.print(current, 3);
        Serial.print(" A, Power: ");
        Serial.print(power, 3);
        Serial.print(" W, Energy: ");
        Serial.print(energy, 3);
        Serial.println(" Wh");

        delay(2000);  
      server.handleClient(); // Handle incoming client requests
    }

    // Root handler (optional welcome message)
    void handleRoot() {
      server.send(200, "text/plain", "ESP32 Home Automation Server");
    }

   
    void handleFanToggle() {
      fanState = !fanState;  // Toggle the fan state
      digitalWrite(fanPin, fanState ? HIGH : LOW);
      
      // Send the new state as a response
      String message = fanState ? "OFF" : "ON"; 
      server.send(200, "text/plain", message);
      
      Serial.println(fanState ? "Fan is OFF" : "Fan is ON");
    }

    void handleLightToggle() {
      lightState = !lightState;
      digitalWrite(lightPin, lightState ? HIGH : LOW);
      String message = lightState ? "OFF" : "ON";
      server.send(200, "text/plain",message); // Send "ON" or "OFF" state
      
    }

    void handleHeaterToggle() {
      heaterState = !heaterState;
      digitalWrite(heaterPin, heaterState ? HIGH : LOW);
      String message = heaterState ? "OFF" : "ON";
      server.send(200, "text/plain", message); // Send "ON" or "OFF" state
      Serial.println(message);
    }
    void handleUvToggle() {
      UvState = !UvState;
      digitalWrite(UvPin, UvState ? HIGH : LOW);
      String message = UvState ? "OFF" : "ON";
      server.send(200, "text/plain", message); // Send "ON" or "OFF" state
      Serial.println(message);
    }
    void handleMforward() {
      motorForward = !motorForward;
      digitalWrite(m1, motorForward ? HIGH : LOW);
      String message = motorForward ? "OFF" : "ON";
      server.send(200, "text/plain", message); // Send "ON" or "OFF" state
      Serial.println(message);
    }
    void handleMbackward() {
      motorbackward = !motorbackward;
      digitalWrite(m2, motorbackward ? HIGH : LOW);
      String message = motorbackward ? "OFF" : "ON";
      server.send(200, "text/plain", message); // Send "ON" or "OFF" state
      Serial.println(message);
    }
    void handleConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing body\"}");
    return;
  }

  String body = server.arg("plain");
  Serial.println("Received config: " + body);

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  receivedSSID = doc["ssid"].as<String>();
  receivedPassword = doc["password"].as<String>();
  userId = doc["user_id"].as<String>();

  server.send(200, "application/json", "{\"message\":\"Credentials received\"}");
  connectToWiFi();
}
void connectToWiFi() {
  Serial.println("Connecting to WiFi: " + receivedSSID);
  WiFi.begin(receivedSSID.c_str(), receivedPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
    sendToSupabase();
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}
void saveCredentials() {
  saveStringToEEPROM(0, receivedSSID);
  saveStringToEEPROM(33, receivedPassword);
  saveStringToEEPROM(66, userId);
  EEPROM.commit();
  Serial.println("✅ Credentials saved to EEPROM.");
}
void loadCredentials() {
  receivedSSID = readStringFromEEPROM(0);
  receivedPassword = readStringFromEEPROM(33);
  userId = readStringFromEEPROM(66);
}
void sendToSupabase() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // Use for dev only

    HTTPClient https;

    if (https.begin(client, supabaseUrl)) {
      https.addHeader("Content-Type", "application/json");
      https.addHeader("apikey", supabaseKey);
      https.addHeader("Authorization", "Bearer " + String(supabaseKey));
      https.addHeader("Prefer", "resolution=merge-duplicates");

      DynamicJsonDocument doc(256);
      doc["mac_address"] = WiFi.macAddress();
      doc["esp_address"] = WiFi.localIP().toString();
      doc["user_id"] = userId;

      String jsonBody;
      serializeJson(doc, jsonBody);

      int responseCode = https.POST(jsonBody);
      String response = https.getString();

      Serial.println("Supabase response code: " + String(responseCode));
      Serial.println("Supabase response: " + response);

      https.end();
      saveCredentials();
    } else {
      Serial.println("❌ HTTPS connection failed.");
    }
  }
}
void saveStringToEEPROM(int startAddr, const String& str) {
  int len = str.length();
  EEPROM.write(startAddr, len);  // Store length
  for (int i = 0; i < len; i++) {
    EEPROM.write(startAddr + 1 + i, str[i]);
  }
}
String readStringFromEEPROM(int startAddr) {
  int len = EEPROM.read(startAddr);
  String str = "";
  for (int i = 0; i < len; i++) {
    str += char(EEPROM.read(startAddr + 1 + i));
  }
  return str;
}


