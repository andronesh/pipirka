#include <Arduino.h>

/*
 * Barcode Scanner to ESP32-C3 (Serial Mode)
 */

#define SCANNER_RX 20  // Connect to Scanner TX (Green)
#define SCANNER_TX 21  // Connect to Scanner RX (White)

void setup() {
  // PC Debugging
  Serial.begin(115200);
  
  // Scanner Communication
  // Note: Using 'Serial1' for the scanner so 'Serial' stays free for your PC
  Serial1.begin(9600, SERIAL_8N1, SCANNER_RX, SCANNER_TX);
  
  Serial.println("--- System Initialized ---");
  Serial.println("Ready to scan...");
}

void loop() {
  // Check if data is coming from the scanner
  if (Serial1.available() > 0) {
    // Read the incoming string until a newline or carriage return
    String barcode = Serial1.readStringUntil('\r');
    
    if (barcode.length() > 0) {
      Serial.print("Scanned Code: ");
      Serial.println(barcode);
    }
  }
}