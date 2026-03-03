#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

int scanTime = 5; // Scan duration in seconds
BLEScan* pBLEScan;

void setup_old() {
  Serial.begin(115200);
  Serial.println("Initializing BLE Scanner...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); // Create new scan
  pBLEScan->setActiveScan(true);   // Active scan uses more power but gets more info
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);         // Less than or equal to interval
}

void loop_old() {
  Serial.println("Starting scan...");
  
  // Start scan and get results
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  
  // Iterate through found devices
  for (int i = 0; i < foundDevices.getCount(); i++) {
    BLEAdvertisedDevice device = foundDevices.getDevice(i);
    Serial.printf("Device %d: %s (Address: %s, RSSI: %d)\n", 
                  i + 1, 
                  device.getName().c_str(), 
                  device.getAddress().toString().c_str(), 
                  device.getRSSI());
  }

  pBLEScan->clearResults(); // Clear results to free memory for next scan
  delay(2000);              // Wait 2 seconds before next scan
}