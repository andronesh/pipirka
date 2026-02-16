#include <WiFi.h>

const char* ssid = "PIPIRKA";
const char* password = "12345678";
const int ledPin = 4;

void onDeviceConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("\n--- New Device Detected! ---");
  
  // We can even see the MAC address of the device that joined
  Serial.print("Device MAC: ");
for(int i = 0; i < 6; i++) {
    // Change 'mac' to 'bssid'
    Serial.printf("%02X", info.wifi_sta_connected.bssid[i]); 
    if(i < 5) Serial.print(":");
  }
  Serial.println("\n----------------------------");
}

void setup() {
  Serial.begin(9600);
  
  // Start the Access Point
  WiFi.softAP(ssid, password);

  WiFi.onEvent(onDeviceConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

  Serial.println("Access Point Started!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  pinMode(ledPin, OUTPUT);
}

void loop() {
  Serial.println("BIG PIPIRKA is running...");
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
}