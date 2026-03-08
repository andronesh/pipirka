#include <NimBLEDevice.h>
#include <Adafruit_GFX.h>

// Printer UUIDs
static const char* SERVICE_UUID = "ff00"; 
static const char* CHAR_UUID    = "ff02";

NimBLEClient* pClient;
NimBLERemoteCharacteristic* pRemoteChar;

// D30 Print Buffer (96 pixels wide / 8 bits per byte = 12 bytes per row)
const int width = 96;
const int height = 64; 
uint8_t bitmap[12 * height];

int counter = 0;
int devicesFound = 0;
// NimBLEAdvertisedDevice printerDevice;

const char* PRINTER_MAC = "a2:ba:cb:2f:bd:e1";

void scanForDevices() {
   NimBLEScan* pScan = NimBLEDevice::getScan();
   pScan->setActiveScan(true);
   pScan->setInterval(1000);
   pScan->setWindow(999);

   Serial.println("   starting scan...");
   if (pScan->start(2002, false, true)) {
      Serial.println("   scan started successfully. Processing results...");
      NimBLEScanResults results = pScan->getResults(); 
      Serial.printf("   --- devices found: %d\n", results.getCount());

      devicesFound = results.getCount();
      for (int i = 0; i < results.getCount(); i++) {
         const NimBLEAdvertisedDevice* device = results.getDevice(i); 
         Serial.printf("      device %s: %s (RSSI: %d)\n", device->getName().c_str(), device->getAddress().toString().c_str(), device->getRSSI());
      }
   } else {
      Serial.println("Scan failed to start.");
   }
}

void startScan() {
   NimBLEDevice::init("ESP32_C3_Labeler");
   NimBLEDevice::setPower(ESP_PWR_LVL_P9);
   NimBLEScan* pScan = NimBLEDevice::getScan();
   if (pScan->isScanning()) {
      Serial.println("Already scanning, skipping new scan.");
      return;
   }
   pScan->setActiveScan(true);
   pScan->setInterval(2000);
   pScan->setWindow(1999);
   Serial.println("   starting scan...");
   if (pScan->start(20002, false, true)) {
      Serial.println("   scan started successfully. Processing results...");
   } else {
      Serial.println("   scan failed to start.");
   }
}

void connectToPrinter() {
      NimBLEScan* pScan = NimBLEDevice::getScan();
      NimBLEScanResults results = pScan->getResults(); 
      Serial.printf("   devices found: %d\n", results.getCount());

      for (int i = 0; i < results.getCount(); i++) {
         const NimBLEAdvertisedDevice* device = results.getDevice(i); 
         Serial.printf("      device %s: %s (RSSI: %d)\n", device->getName().c_str(), device->getAddress().toString().c_str(), device->getRSSI());

         if (device->getName() == "Q30") {
            Serial.print("          PRINTER found: ");
            Serial.println(device->getAddress().toString().c_str());

            pClient = NimBLEDevice::createClient();

            // Use the pointer directly in connect()
            // if (pClient->connect(device)) {
            //    Serial.println("Connected to Phomemo D30!");

            //    // // Set a larger MTU to send image data faster
            //    // pClient->setMTU(512); 

            //    auto service = pClient->getService(SERVICE_UUID);
            //    if (service) {
            //       pRemoteChar = service->getCharacteristic(CHAR_UUID);
            //    }
            //    return;
            // }
            if (pClient->connect(device)) { 
               Serial.println("Connected to Phomemo D30!");

               // Instead of pClient->setMTU, NimBLE handles this automatically.
               // Let's grab the service:
               NimBLERemoteService* pService = pClient->getService("ff00");
               if (pService) {
                  pRemoteChar = pService->getCharacteristic("ff02");
                  if (pRemoteChar) {
                     Serial.println("Ready to print!");
                  } else {
                     Serial.println("Failed to find characteristic ff02");
                  }
               } else {
                  Serial.println("Failed to find service ff00");
               }
               return;
            }
         }
      }
}

void printLabel(const char* text) {
   if (!pRemoteChar) {
      Serial.println("--- Characteristic not found, cannot print.");
      return;
   }

   // 1. Prepare the ESC/POS Header (Found in the polskafan repo logic)
   // Format: GS v 0 m xL xH yL yH
   uint8_t header[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, (uint8_t)height, 0x00};

   // 2. Send Header
   pRemoteChar->writeValue(header, sizeof(header), true);

   // 3. Send Bitmap Data (This is where the text pixels go)
   // Note: You'd use Adafruit_GFX to fill the 'bitmap' array here
   pRemoteChar->writeValue(bitmap, sizeof(bitmap), true);

   // 4. Paper Feed command
   uint8_t feed[] = {0x1b, 0x4a, 0x40}; 
   pRemoteChar->writeValue(feed, sizeof(feed), true);
   Serial.println("--- Label sent to printer!");
}

void printLabel2(const char* text) {
   if (!pRemoteChar) return;

   Serial.println(">>> Sending Motor Test...");

   // These are standard Phomemo "Feed" commands (hex)
   // 1b 40 (Initialize)
   // 1b 64 04 (Feed 4 lines)
   uint8_t initCmd[] = {0x1b, 0x40};
   uint8_t feedCmd[] = {0x1b, 0x64, 0x04};

   pRemoteChar->writeValue(initCmd, sizeof(initCmd), true);
   delay(100);
   pRemoteChar->writeValue(feedCmd, sizeof(feedCmd), true);

   Serial.println(">>> Motor commands sent!");
}

// void printLabel3(const char* text) {
//     if (!pRemoteChar) return;

//    const int CANVAS_WIDTH = 96;
//    const int CANVAS_HEIGHT = 64;
//    GFXcanvas1 canvas(CANVAS_WIDTH, CANVAS_HEIGHT);

//     // 1. Draw on the Canvas
//     canvas.fillScreen(0);               // Clear canvas (0 = white)
//     canvas.setCursor(0, 10);            // Move "pen" to x=0, y=10
//     canvas.setTextColor(1);             // 1 = black
//     canvas.setTextSize(22);              // Set font size
//     canvas.println(text);               // Write the string

//     // 2. Prepare Phomemo ESC/POS Header
//     // 0x1d 0x76 0x30 0x00 (Command)
//     // 0x0c 0x00 (12 bytes wide)
//     // 0x40 0x00 (64 pixels high)
//     uint8_t header[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0x40, 0x00};

//     // 3. Send Start Job command (Required by some Q30 models)
//     uint8_t startJob[] = {0x1f, 0x11, 0x11};
//     pRemoteChar->writeValue(startJob, sizeof(startJob), true);

//     // 4. Send Header
//     pRemoteChar->writeValue(header, sizeof(header), true);

//     // 5. Send Canvas Data
//     // canvas.getBuffer() returns the raw bits we just "drew"
//     size_t bufferSize = (CANVAS_WIDTH * CANVAS_HEIGHT) / 8;
//     pRemoteChar->writeValue(canvas.getBuffer(), bufferSize, true);

//     // 6. Send Feed & End Job
//     uint8_t feed[] = {0x1b, 0x4a, 0x40}; 
//     uint8_t endJob[] = {0x1f, 0x11, 0x12};
    
//     pRemoteChar->writeValue(feed, sizeof(feed), true);
//     pRemoteChar->writeValue(endJob, sizeof(endJob), true);

//     Serial.println(">>> Label printed!");
// }

// most working, but label is only 1/3 of the widht
void printLabel3(const char* text) {
   if (!pRemoteChar) return;

   // D30/Q30 Width is 96 pixels. 
   // Height can be whatever you want, let's stick to 64 for a small label.
   const int CANVAS_WIDTH = 96;
   const int CANVAS_HEIGHT = 64;

   // Create the virtual canvas
   GFXcanvas1 canvas(CANVAS_WIDTH, CANVAS_HEIGHT);

   // 1. Draw "HELLO" onto the canvas
   canvas.fillScreen(1);               // White background
   canvas.setRotation(0);             // Landscape mode
   canvas.setCursor(5, 10);            // Padding to avoid the edge
   canvas.setTextColor(0);             // Black text
   canvas.setTextSize(2);
   canvas.print(text);

   // 2. Protocol Initialization
   uint8_t init[] = {0x1b, 0x40};      // ESC @ (Initialize)
   pRemoteChar->writeValue(init, sizeof(init), true);

   // 3. Image Header: GS v 0 m xL xH yL yH
   // Width: 96px (12 bytes), Height: 64px
   uint8_t header[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0x40, 0x00};
   pRemoteChar->writeValue(header, sizeof(header), true);

   // 4. Send Buffer in chunks (BLE safety)
   uint8_t* buf = canvas.getBuffer();
   size_t totalLen = (CANVAS_WIDTH * CANVAS_HEIGHT) / 8;
   size_t chunkSize = 20; // Standard BLE packet limit

   for (size_t i = 0; i < totalLen; i += chunkSize) {
      size_t size = min(chunkSize, totalLen - i);
      pRemoteChar->writeValue(buf + i, size, true);
      delay(10); // Give the printer time to process the chunk
   }

   // 5. Feed Command: ESC J n (Feed n dots)
   uint8_t feed[] = {0x1b, 0x4a, 0x60}; 
   pRemoteChar->writeValue(feed, sizeof(feed), true);

   Serial.println(">>> Data chunked and sent!");
}

uint8_t reverse(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}


// empty label
void printLabel4(const char* text) {
    if (!pRemoteChar) return;

    // D30/Q30 Width is 96 pixels. 
      // Height can be whatever you want, let's stick to 64 for a small label.
      const int CANVAS_WIDTH = 96;
      const int CANVAS_HEIGHT = 200;

      // Create the virtual canvas
      GFXcanvas1 canvas(CANVAS_WIDTH, CANVAS_HEIGHT);

    // Draw
    canvas.fillScreen(1);
    canvas.setTextColor(0);
   //  canvas.setRotation(1);
    canvas.setTextSize(8);
    canvas.setCursor(20, 30);
    canvas.print(text);

    // 1. Wake up
    uint8_t start[] = {0x1f, 0x11, 0x11};
    pRemoteChar->writeValue(start, sizeof(start), true);
    delay(20);

    // 2. Header (Height set to 240)
    uint8_t header[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0xf0, 0x00};
    pRemoteChar->writeValue(header, sizeof(header), true);
    delay(88);

    // 3. Data Chunks
    uint8_t* buf = canvas.getBuffer();
    for (size_t i = 0; i < (96 * 240 / 8); i += 20) {
        pRemoteChar->writeValue(buf + i, min((size_t)20, (96 * 240 / 8) - i), true);
        delay(2); 
    }

    // 4. THE FIX: End Data and Auto-Gap Feed
    uint8_t finish[] = {0x1f, 0x11, 0x12, 0x1b, 0x64, 0x01}; 
    pRemoteChar->writeValue(finish, sizeof(finish), true);
    Serial.println(">>> Label sent to Q30!");
}

// empty labels
void printLabel5(const char* text) {
   if (!pRemoteChar) return;

   const int PHYS_W = 96; 
   const int PHYS_H = 240; 
   GFXcanvas1 canvas(PHYS_W, PHYS_H);

   // Draw "HELLO"
   canvas.fillScreen(0);               
   canvas.setRotation(1);              
   canvas.setTextColor(1);             
   canvas.setTextSize(4); // Bigger to be sure
   canvas.setCursor(20, 30); 
   canvas.print(text);

   // 1. POWER UP / WAKE SEQUENCE
   uint8_t wake[] = {0x1b, 0x40, 0x1f, 0x11, 0x11}; 
   pRemoteChar->writeValue(wake, sizeof(wake), true);
   delay(100);

   // 2. IMAGE HEADER (GS v 0)
   uint8_t header[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0xf0, 0x00};
   pRemoteChar->writeValue(header, sizeof(header), true);
   delay(50);

   // 3. SEND DATA WITH BIT REVERSAL
   uint8_t* buf = canvas.getBuffer();
   size_t totalLen = (PHYS_W * PHYS_H) / 8;
   uint8_t chunk[20];
   size_t chunkIdx = 0;

   for (size_t i = 0; i < totalLen; i++) {
      // REVERSE THE BIT ORDER HERE
      chunk[chunkIdx++] = reverse(buf[i]);
      chunk[chunkIdx++] = 0xFF;

      if (chunkIdx == 20 || i == totalLen - 1) {
         pRemoteChar->writeValue(chunk, chunkIdx, true);
         chunkIdx = 0;
         delay(8); // Small delay for printer CPU
      }
   }

   // 4. PRINT AND EJECT
   uint8_t finish[] = {0x1f, 0x11, 0x12, 0x1b, 0x64, 0x01}; 
   pRemoteChar->writeValue(finish, sizeof(finish), true);
   
   Serial.println(">>> Label sent with Bit Reversal!");
}


// infinite labels
void printLabel6(const char* text) {
   if (!pRemoteChar) return;

   const int PHYS_W = 96; 
   const int PHYS_H = 240; 
   GFXcanvas1 canvas(PHYS_W, PHYS_H);

   canvas.fillScreen(0);               
   canvas.setRotation(1);              
   canvas.setTextColor(1);             
   canvas.setTextSize(4); 
   canvas.setCursor(20, 30); 
   canvas.print(text);

   // 1. WAKE UP & SET COMPRESSION OFF (0x1f 0x11 0x13 0x00)
   uint8_t init[] = {0x1f, 0x11, 0x11, 0x1f, 0x11, 0x13, 0x00}; 
   pRemoteChar->writeValue(init, sizeof(init), true);
   delay(100);

   // 2. SEND DATA ROW BY ROW
   // Command for a single line: 0x1f 0x10 (line_index) (data_bytes)
   uint8_t* buf = canvas.getBuffer();
   
   for (int y = 0; y < PHYS_H; y++) {
      uint8_t lineHeader[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0x01, 0x00};
      pRemoteChar->writeValue(lineHeader, sizeof(lineHeader), true);
      
      // Each row is 12 bytes (96 pixels / 8)
      uint8_t rowData[12];
      for(int x = 0; x < 12; x++) {
          rowData[x] = buf[y * 12 + x]; 
          // If this is still empty, try: rowData[x] = ~buf[y * 12 + x]; (Invert)
      }
      
      pRemoteChar->writeValue(rowData, 12, true);
      delay(5); // Essential for the head to heat up
   }

   // 3. EJECT
   uint8_t finish[] = {0x1f, 0x11, 0x12, 0x1b, 0x64, 0x01}; 
   pRemoteChar->writeValue(finish, sizeof(finish), true);
   
   Serial.println(">>> Line-by-Line Sent!");
}

void printStripeTest() {
   if (!pRemoteChar) return;

   Serial.println(">>> Starting Stripe Test...");

   // 1. Wake up and clear buffer
   uint8_t init[] = {0x1f, 0x11, 0x11, 0x1b, 0x40}; 
   pRemoteChar->writeValue(init, sizeof(init), true);
   delay(100);

   // 2. Single Header for the WHOLE image
   // Width: 12 bytes (96px), Height: 100 px
   uint8_t header[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0x64, 0x00};
   pRemoteChar->writeValue(header, sizeof(header), true);
   delay(50);

   // 3. Send 100 lines of alternating bits (Stripes)
   uint8_t stripe[12];
   memset(stripe, 0xAA, 12); // Binary 10101010

   for (int i = 0; i < 100; i++) {
      pRemoteChar->writeValue(stripe, 12, true);
      // NO DELAY here, or very small
      delay(2); 
   }

   // 4. End Job and Feed
   uint8_t finish[] = {0x1f, 0x11, 0x12, 0x1b, 0x64, 0x01}; 
   pRemoteChar->writeValue(finish, sizeof(finish), true);
   
   Serial.println(">>> Stripe test sent!");
}

void printLabel7(const char* text) {

  if (!pRemoteChar) return;

  const int CANVAS_WIDTH  = 96;    // Q30 raster width
  const int CANVAS_HEIGHT = 120;   // can change

  GFXcanvas1 canvas(CANVAS_WIDTH, CANVAS_HEIGHT);

  // Draw
  canvas.fillScreen(1);        // white
  canvas.setTextColor(0);      // black
  canvas.setTextSize(2);
  canvas.setCursor(5, 30);
  canvas.print(text);

  uint8_t* buf = canvas.getBuffer();
  size_t totalLen = (CANVAS_WIDTH * CANVAS_HEIGHT) / 8;

  // Invert bits
  for (size_t i = 0; i < totalLen; i++) {
    buf[i] = ~buf[i];
  }

  // Init
  uint8_t init[] = {0x1B, 0x40};
  pRemoteChar->writeValue(init, sizeof(init), true);
  delay(100);

  // Header (96px = 12 bytes wide)
  uint8_t header[] = {
    0x1D, 0x76, 0x30, 0x00,
    0x0C, 0x00,                        // 12 bytes width
    (uint8_t)(CANVAS_HEIGHT & 0xFF),
    (uint8_t)((CANVAS_HEIGHT >> 8) & 0xFF)
  };

  pRemoteChar->writeValue(header, sizeof(header), true);
  delay(20);

  // Send small chunks (Q30 BLE is picky)
  const size_t chunkSize = 20;

  for (size_t i = 0; i < totalLen; i += chunkSize) {
    size_t len = min(chunkSize, totalLen - i);
    pRemoteChar->writeValue(buf + i, len, true);
    delay(10);
  }

  delay(50);

  // Feed
  uint8_t feed[] = {0x1B, 0x4A, 0x40};
  pRemoteChar->writeValue(feed, sizeof(feed), true);

  Serial.println(">>> Done");
}

void printLabelPolska(const char* text) {
    if (!pRemoteChar) return;

    const int PHYS_W = 96; 
    const int PHYS_H = 240; // 30mm length
    GFXcanvas1 canvas(PHYS_W, PHYS_H);

    // 1. Draw (Inverting here to be safe: 0=White, 1=Black)
    canvas.fillScreen(0);
    canvas.setRotation(1);
    canvas.setTextSize(3);
    canvas.setTextColor(1);
    canvas.setCursor(20, 30);
    canvas.print(text);

    // 2. WAKE UP (From the repo: 1f 11 11)
    uint8_t startCmd[] = {0x1f, 0x11, 0x11};
    pRemoteChar->writeValue(startCmd, sizeof(startCmd), true);
    delay(50);

    // 3. SEND LINE BY LINE (The "Polskafan" way)
    uint8_t* buf = canvas.getBuffer();
    
    for (uint16_t y = 0; y < PHYS_H; y++) {
        // The repo uses 1f 10 + 2-byte line index
        uint8_t lineHeader[4];
        lineHeader[0] = 0x1f;
        lineHeader[1] = 0x11; // Some versions use 0x11, others 0x10. Try 0x10 if 0x11 fails.
        lineHeader[2] = (y >> 8) & 0xFF; // High byte of index
        lineHeader[3] = y & 0xFF;        // Low byte of index
        
        // Combine header + 12 bytes of data into one 16-byte packet
        uint8_t packet[16];
        memcpy(packet, lineHeader, 4);
        
        for(int x = 0; x < 12; x++) {
            // IMPORTANT: The repo uses bit-inversion (NOT operator)
            packet[4 + x] = ~buf[y * 12 + x]; 
        }

        pRemoteChar->writeValue(packet, 16, true);
        delay(2); // Small pacing for BLE
    }

    // 4. FINISH (From the repo: 1f 11 12 followed by 1f 11 0e)
    uint8_t endCmd[] = {0x1f, 0x11, 0x12};
    uint8_t feedCmd[] = {0x1f, 0x11, 0x0e};
    
    pRemoteChar->writeValue(endCmd, sizeof(endCmd), true);
    delay(50);
    pRemoteChar->writeValue(feedCmd, sizeof(feedCmd), true);

    Serial.println(">>> Polskafan-style label sent!");
}

void sendPhomemoPacket(uint8_t* data, uint16_t len) {

  uint16_t totalLen = len + 5;

  uint8_t packet[totalLen + 6];

  packet[0] = 0x51;
  packet[1] = 0x78;

  packet[2] = totalLen & 0xFF;
  packet[3] = (totalLen >> 8) & 0xFF;

  packet[4] = 0x00;
  packet[5] = 0x00;
  packet[6] = 0xA5;

  memcpy(&packet[7], data, len);

  uint8_t crc = 0;
  for (int i = 6; i < 7 + len; i++) {
    crc ^= packet[i];
  }

  packet[7 + len] = crc;

  pRemoteChar->writeValue(packet, 8 + len, true);
}

void testFeed() {

  if (!pRemoteChar) return;

  // Simple feed command
  uint8_t escFeed[] = {0x1B, 0x4A, 0x60};

  sendPhomemoPacket(escFeed, sizeof(escFeed));

  Serial.println("Feed test sent");
}

void connectToAddress() {
    Serial.println("Attempting manual connection...");

    // 1. Create a NimBLEAddress object (Address type 1 is common for BLE printers)
   //  NimBLEAddress printerAddr(PRINTER_MAC, 1); 
    NimBLEAddress printerAddr(PRINTER_MAC, 0); 

    // 2. Create the client
    pClient = NimBLEDevice::createClient();

    // 3. Connect directly using the address
    // This skips scanning and tries to open the "pipe" immediately
    if (pClient->connect(printerAddr)) {
        Serial.println("Success! Connected to Q30 via MAC.");
        
        // After connecting, find the "Print" characteristic
        auto pService = pClient->getService("ff00");
        if (pService) {
            Serial.println("   Found service ff00:");
            Serial.printf("      uuid: %s\n", pService->getUUID().toString().c_str());
            pRemoteChar = pService->getCharacteristic("ff02");
            if (pRemoteChar) {
                Serial.println("   Found characteristic ff02:");
                Serial.printf("      uuid: %s\n", pRemoteChar->getUUID().toString().c_str());
                printLabel3("HELLO");
               //  printStripeTest();
               // testFeed();
            }
        }
    } else {
        Serial.println("Failed to connect. If it keeps failing, try address type 0.");
    }
}

void setup() {
   Serial.begin(115200);
   delay(2002);
   startScan();
}

void loop() {
   delay(888);
   counter++;
   if (pRemoteChar) {
      Serial.printf("Should be ready to print, counter: %d\n", counter);
      if (counter == 8) {
         Serial.println("   sending label...");
         printLabel3("HELLO");
      }
   } else {
      Serial.println("Not connected to printer yet, trying again...");
      connectToPrinter();
   }
}
