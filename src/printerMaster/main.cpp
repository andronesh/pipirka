#include <NimBLEDevice.h>
#include <Adafruit_GFX.h>
#include <FS.h>
#include <LittleFS.h>

NimBLEClient* pClient;
NimBLERemoteCharacteristic* pRemoteChar;
NimBLERemoteCharacteristic* notifyChar;

int counter = 0;
int devicesFound = 0;

const char* PRINTER_MAC = "a2:ba:cb:2f:bd:e1";


// doesn't trigger
void notifyCallback(
    BLERemoteCharacteristic* chr,
    uint8_t* data,
    size_t len,
    bool isNotify)
{
    Serial.print("---> Printer status: ");

    for (int i = 0; i < len; i++) {
        Serial.printf("%02X ", data[i]);
    }

    Serial.println();
}

void setupNotification(NimBLERemoteService* service) {
    notifyChar = service->getCharacteristic("0000ff01-0000-1000-8000-00805f9b34fb");
    if (notifyChar) {
       bool subscribed = notifyChar->subscribe(true, notifyCallback);
        if (subscribed) {
            Serial.println("Subscribed to printer status notifications");
         } else {
            Serial.println("Failed to subscribe to printer notifications");
         }
    } else {
        Serial.println("Failed to find notify characteristic");
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

            if (pClient->connect(device)) { 
               pClient->updateConnParams(6, 6, 0, 60);
               NimBLEDevice::setMTU(512);
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
                  setupNotification(pService);
                  // BLERemoteCharacteristic* pStatusChar = pService->getCharacteristic("ff01");
                  // if (pStatusChar && pStatusChar->canNotify()) {
                  //    // 2. Subscribe to it. This tells the printer "I am an active app"
                  //    pStatusChar->subscribe(true, [] (BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
                  //       // You can print the printer's status here (Battery, paper, etc.)
                  //       Serial.print("Printer Status Update: ");
                  //       for(int i=0; i<length; i++) Serial.printf("%02X ", pData[i]);
                  //       Serial.println();
                  //    });
                  // }
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

   // D30 Print Buffer (96 pixels wide / 8 bits per byte = 12 bytes per row)
   const int width = 96;
   const int height = 64; 
   uint8_t bitmap[12 * height];

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
   const int CANVAS_HEIGHT = 240;

   // Create the virtual canvas
   GFXcanvas1 canvas(CANVAS_WIDTH, CANVAS_HEIGHT);

   // 1. Draw "HELLO" onto the canvas
   canvas.fillScreen(1);               // White background
   canvas.setRotation(1);             // Landscape mode
   canvas.setCursor(25, 33);            // Padding to avoid the edge
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

   for (size_t i = 0; i < totalLen; i += chunkSize) {
      size_t size = min(chunkSize, totalLen - i);
      pRemoteChar->writeValue(buf + i, size, true);
      delay(10); // Give the printer time to process the chunk
   }

   for (size_t i = 0; i < totalLen; i += chunkSize) {
      size_t size = min(chunkSize, totalLen - i);
      pRemoteChar->writeValue(buf + i, size, true);
      delay(10); // Give the printer time to process the chunk
   }


   // // 5. Feed Command: ESC J n (Feed n dots)
   // uint8_t feed[] = {0x1b, 0x4a, 0x60}; 
   // pRemoteChar->writeValue(feed, sizeof(feed), true);

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

// max copy of polskafan repo, empty label
void printPolskafanStripes() {
    if (!pRemoteChar) return;

    // 2. THE SNIFFED INITIALIZATION (Lines 26-34)
   //  packets = [
   //      '1f1138',
   //      '1f11121f1113',
   //      '1f1109',
   //      '1f1111',
   //      '1f1119',
   //      '1f1107',
   //      '1f110a1f110202'
   //  ]

    uint8_t sniffedHeader[] = {
        0x1f, 0x11, 0x38,
        0x1f, 0x11, 0x12, 0x1f, 0x11, 0x13,
        0x1f, 0x11, 0x09,
        0x1f, 0x11, 0x11,
        0x1f, 0x11, 0x19,
        0x1f, 0x11, 0x07,
        0x1f, 0x11, 0x0a, 0x1f, 0x11, 0x02, 0x02
    };
    
    // Send the "Unlock" and "Header" together
    pRemoteChar->writeValue(sniffedHeader, sizeof(sniffedHeader), true);
    delay(100);

    // send image prefix output = '1f1124001b401d7630000c004001'
        // 1f 11 24 00 (ID) + 1b 40 (Init) + 1d 76 30 00 (Raster) + 0c 00 (W) + f0 00 (H)
    uint8_t imagePrefix[] = {
        0x1f, 0x11, 0x24, 0x00, 
        0x1b, 0x40, 
        0x1d, 0x76, 0x30, 0x00, 
        0x0c, 0x00, 
        0x30, 0x00 // 320px height
    };
    pRemoteChar->writeValue(imagePrefix, sizeof(imagePrefix), true);
     delay(100);

   const int height = 255; // Adjust based on your image
   const int width = 96;   // Adjust based on your image
   // uint8_t packedRowBlack[12] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
   // uint8_t packedRowWhite[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

   // int lineColorCounter = 0;
   // bool lineIsBlack = true;

   // for (size_t i = 0; i < height; i++) {
   //    // if  (i % 64 == 0) {
   //    //    pRemoteChar->writeValue(imagePrefix, sizeof(imagePrefix), true);
   //    //    Serial.printf("--- Sent IMAGE PREFIX line %d\n", i);
   //    // }
   //    if (lineIsBlack) {
   //       pRemoteChar->writeValue(packedRowBlack, 12, true);
   //       Serial.printf(">>> Sent BLACK line %d\n", i);
   //    } else {
   //       pRemoteChar->writeValue(packedRowWhite, 12, true);
   //       Serial.printf(">>> Sent WHITE line %d\n", i);
   //    }
   //     lineColorCounter++;
   //     if (lineColorCounter >= 5) { // Change color every 5 lines
   //        lineIsBlack = !lineIsBlack;
   //        lineColorCounter = 0;
   //     }
   //    delay(10); // Give the printer time to process the chunk
   // }

   uint8_t packedData[12 * height]; // The result (1 per bit)

   int lineColorCounter  = 0;
   bool lineIsBlack = false;

   int byteIdx = 0;

    for (int y = 0; y < height; y++) {
        for (int byteNum = 0; byteNum < (width / 8); byteNum++) {
            uint8_t currentByte = 0;

            for (int bit = 0; bit < 8; bit++) {
               //  if (lineIsBlack) {
               //      currentByte |= (1 << (7 - bit));
               //  }
               if (byteNum % 2 == 0) { // Even byte: black
                    currentByte |= (1 << (7 - bit));
                }
            }
            packedData[byteIdx++] = currentByte;
        }
         lineColorCounter++;
         if (lineColorCounter >= 5) {
            lineIsBlack = !lineIsBlack;
            lineColorCounter = 0;
         }
    }

   size_t totalLen = sizeof(packedData);
   size_t chunkSize = 20; // Standard BLE packet limit

   for (size_t i = 0; i < totalLen; i += chunkSize) {
      size_t size = min(chunkSize, totalLen - i);
      pRemoteChar->writeValue(packedData + i, size, true);
      delay(10); // Give the printer time to process the chunk
   }

    Serial.println(">>> Sniffed protocol label sent!");
}

void printPolskafanStripes2() {
    if (!pRemoteChar) return;

    // 2. THE SNIFFED INITIALIZATION (Lines 26-34)
   //  packets = [
   //      '1f1138',
   //      '1f11121f1113',
   //      '1f1109',
   //      '1f1111',
   //      '1f1119',
   //      '1f1107',
   //      '1f110a1f110202'
   //  ]

    uint8_t sniffedHeader[] = {
        0x1f, 0x11, 0x38,
        0x1f, 0x11, 0x12, 0x1f, 0x11, 0x13,
        0x1f, 0x11, 0x09,
        0x1f, 0x11, 0x11,
        0x1f, 0x11, 0x19,
        0x1f, 0x11, 0x07,
        0x1f, 0x11, 0x0a, 0x1f, 0x11, 0x02, 0x02
    };
    
    // Send the "Unlock" and "Header" together
    pRemoteChar->writeValue(sniffedHeader, sizeof(sniffedHeader), true);
    delay(100);

    // send image prefix output = '1f1124001b401d7630000c004001'
        // 1f 11 24 00 (ID) + 1b 40 (Init) + 1d 76 30 00 (Raster) + 0c 00 (W) + f0 00 (H)
    uint8_t imagePrefix[] = {
        0x1f, 0x11, 0x24, 0x00, 
        0x1b, 0x40, 
        0x1d, 0x76, 0x30, 0x00, 
        0x0c, 0x00, 
        0x40, 0x00 // Let's use 240 for your 30mm label
    };
    pRemoteChar->writeValue(imagePrefix, sizeof(imagePrefix), true);
    delay(50);

    const int height = 320;
    const int width = 96;
    uint8_t lineData[12]; // Buffer for exactly one row

    for (int y = 0; y < height; y++) {
        // Generate your stripe pattern row-by-row
        for (int byteNum = 0; byteNum < 12; byteNum++) {
            lineData[byteNum] = (byteNum % 2 == 0) ? 0xFF : 0x00;
        }

        // Send one line (12 bytes)
        pRemoteChar->writeValue(lineData, 12, true);

        // THE SECRET SAUCE: Syncing
        // Every 32 lines, give the printer a longer pause to catch up
        if (y > 0 && y % 32 == 0) {
            delay(50); // Give the thermal head time to cool/process
        } else {
            delay(5);  // Minimum pace
        }
    }

    // End Job
   //  uint8_t finish[] = {0x1f, 0x11, 0x12, 0x1b, 0x64, 0x01};
   //  pRemoteChar->writeValue(finish, sizeof(finish), true);

    Serial.println(">>> Sniffed protocol label sent!");
}

// void printImage() {
//    if (!pRemoteChar) return;

//    // 2. Protocol Initialization
//    uint8_t init[] = {0x1b, 0x40};      // ESC @ (Initialize)
//    pRemoteChar->writeValue(init, sizeof(init), true);

//    // 3. Image Header: GS v 0 m xL xH yL yH
//    // Width: 96px (12 bytes), Height: 64px
//    uint8_t header[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0x40, 0x00};
//    pRemoteChar->writeValue(header, sizeof(header), true);

//    File file = LittleFS.open("/imageToPrintNew.png", "r");
//     if (!file) {
//         Serial.println("Failed to open file for reading");
//         return;
//     }

//     size_t fileSize = file.size();
//     uint16_t totalRows = fileSize / 12;

//    // 4. Send Buffer in chunks (BLE safety)
//    uint8_t* buf = canvas.getBuffer();
//    size_t totalLen = (CANVAS_WIDTH * CANVAS_HEIGHT) / 8;
//    size_t chunkSize = 20; // Standard BLE packet limit

//    for (size_t i = 0; i < totalLen; i += chunkSize) {
//       size_t size = min(chunkSize, totalLen - i);
//       pRemoteChar->writeValue(buf + i, size, true);
//       delay(10); // Give the printer time to process the chunk
//    }


//    // // 5. Feed Command: ESC J n (Feed n dots)
//    // uint8_t feed[] = {0x1b, 0x4a, 0x60}; 
//    // pRemoteChar->writeValue(feed, sizeof(feed), true);

//    Serial.println(">>> Data chunked and sent!");
// }

void printStripes() {
   if (!pRemoteChar) return;

   // 2. Protocol Initialization
   uint8_t init[] = {0x1b, 0x40};      // ESC @ (Initialize)
   pRemoteChar->writeValue(init, sizeof(init), true);

   // 3. Image Header: GS v 0 m xL xH yL yH
   // Width: 96px (12 bytes), Height: 64px
   uint8_t header[] = {0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0x40, 0x00};
   pRemoteChar->writeValue(header, sizeof(header), true);


   const int height = 255; // Adjust based on your image
   const int width = 96;   // Adjust based on your image
   // uint8_t rawPixels[96 * 240]; // Your source pixels (1 per byte)
   uint8_t packedData[12 * height]; // The result (1 per bit)

   int lineColorCounter  = 0;
   bool lineIsBlack = false;

   int byteIdx = 0;

    for (int y = 0; y < height; y++) {
        for (int byteNum = 0; byteNum < (width / 8); byteNum++) {
            uint8_t currentByte = 0;

            for (int bit = 0; bit < 8; bit++) {
                if (lineIsBlack) {
                    currentByte |= (1 << (7 - bit));
                }
               // if (byteNum % 2 == 0) { // Even byte: black
               //      currentByte |= (1 << (7 - bit));
               //  }
            }
            packedData[byteIdx++] = currentByte;
        }
         lineColorCounter++;
         if (lineColorCounter >= 5) {
            lineIsBlack = !lineIsBlack;
            lineColorCounter = 0;
         }
    }

   size_t totalLen = sizeof(packedData);
   size_t chunkSize = 20; // Standard BLE packet limit

   for (size_t i = 0; i < totalLen; i += chunkSize) {
      size_t size = min(chunkSize, totalLen - i);
      pRemoteChar->writeValue(packedData + i, size, true);
      delay(10); // Give the printer time to process the chunk
   }

   // // 5. Feed Command: ESC J n (Feed n dots)
   // uint8_t feed[] = {0x1b, 0x4a, 0x60}; 
   // pRemoteChar->writeValue(feed, sizeof(feed), true);

   Serial.println(">>> Data chunked and sent!");
}


// printed 15 empty labels
void printFileToQ30(const char* filename) {
    if (!pRemoteChar) return;

    // 1. Open the file from Flash
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    size_t fileSize = file.size();
    uint16_t totalRows = fileSize / 12; // Assuming 12 bytes per row

    // 2. Wake up / Start Job
    uint8_t start[] = {0x1f, 0x11, 0x11};
    pRemoteChar->writeValue(start, sizeof(start), true);
    delay(50);

    // 3. Send Header (Adjusting height based on file size)
    // yL = totalRows & 0xFF, yH = (totalRows >> 8) & 0xFF
    uint8_t header[] = {
        0x1d, 0x76, 0x30, 0x00, 
        0x0c, 0x00, 
        (uint8_t)(totalRows & 0xFF), (uint8_t)((totalRows >> 8) & 0xFF)
    };
    pRemoteChar->writeValue(header, sizeof(header), true);
    delay(50);

    // 4. Stream bytes directly from file to Bluetooth
    uint8_t buffer[20]; // BLE packet size
    while (file.available()) {
        size_t bytesRead = file.read(buffer, 20);
        
        // Check if we need bit-reversal or inversion (the '~' operator)
        for(size_t i = 0; i < bytesRead; i++) {
            // buffer[i] = ~buffer[i]; // Uncomment if the image is inverted
        }

        pRemoteChar->writeValue(buffer, bytesRead, true);
        delay(4); // Pacing for the Q30
    }

    // 5. Finish and Feed
    uint8_t finish[] = {0x1f, 0x11, 0x12, 0x1b, 0x64, 0x01}; 
    pRemoteChar->writeValue(finish, sizeof(finish), true);

    file.close();
    Serial.println("File printed successfully!");
}

void printPolskafanSync() {
    if (!pRemoteChar) return;

    // 1. FULL INITIALIZATION (Once)
    uint8_t init[] = {0x1f, 0x11, 0x38, 0x1f, 0x11, 0x12, 0x1f, 0x11, 0x13, 0x1f, 0x11, 0x09, 0x1f, 0x11, 0x11, 0x1f, 0x11, 0x19, 0x1f, 0x11, 0x07, 0x1f, 0x11, 0x0a, 0x1f, 0x11, 0x02, 0x02};
    pRemoteChar->writeValue(init, sizeof(init), true);
    delay(200);

    // 2. RASTER HEADER (Setting for 240px)
    // 1f 11 24 00 + 1b 40 + 1d 76 30 00 + 0c 00 + f0 00
    uint8_t rasterHeader[] = {0x1f, 0x11, 0x24, 0x00, 0x1b, 0x40, 0x1d, 0x76, 0x30, 0x00, 0x0c, 0x00, 0xf0, 0x00};
    pRemoteChar->writeValue(rasterHeader, sizeof(rasterHeader), true);
    delay(200);

    const int totalHeight = 240;
    
    // 3. ROW-BY-ROW LOOP
    for (uint16_t y = 0; y < totalHeight; y++) {
        uint8_t packet[16];
        
        // Row Prefix (1f 10 + 2-byte index)
        packet[0] = 0x1f;
        packet[1] = 0x10; 
        packet[2] = (y >> 8) & 0xFF;
        packet[3] = y & 0xFF;

        // Data (12 bytes of stripes for testing)
        for (int b = 0; b < 12; b++) {
            packet[4 + b] = (y % 10 < 5) ? 0xAA : 0x55; 
        }

        // Send the 16-byte packet
        // 'true' forces the ESP32 to wait for the printer to say "Ready"
        pRemoteChar->writeValue(packet, 16, true);

        // FLOW CONTROL
        // Every 8 lines, give it a tiny break to prevent buffer overflow
        if (y % 8 == 0) {
            delay(20); 
        } else {
            delay(2);
        }
    }
    
    Serial.println(">>> Sync label sent!");
}


void printRasterPhomemo(uint8_t* img, int width, int height) {
    if (!pRemoteChar) return;

    const int widthBytes = width / 8;
   //  const int STRIPE_HEIGHT = 255;   // safe size for Phomemo
    const int STRIPE_HEIGHT = height;   // safe size for Phomemo

    // --- printer init sequence (from sniffed packets)
    uint8_t init[] = {
        0x1f,0x11,0x38,
        0x1f,0x11,0x12,0x1f,0x11,0x13,
        0x1f,0x11,0x09,
        0x1f,0x11,0x11,
        0x1f,0x11,0x19,
        0x1f,0x11,0x07,
        0x1f,0x11,0x0a,0x1f,0x11,0x02,0x02
    };

    pRemoteChar->writeValue(init, sizeof(init), false);
    delay(50);

    int y = 0;

    while (y < height) {

        int stripeHeight = min(STRIPE_HEIGHT, height - y);

        uint8_t prefix[] = {
            0x1f,0x11,0x24,0x00,
            0x1b,0x40,
            0x1d,0x76,0x30,0x00,
            (uint8_t)(widthBytes & 0xff),
            (uint8_t)((widthBytes >> 8) & 0xff),
            (uint8_t)(stripeHeight & 0xff),
            (uint8_t)((stripeHeight >> 8) & 0xff)
        };

        pRemoteChar->writeValue(prefix, sizeof(prefix), false);
        delay(10);

        int stripeBytes = stripeHeight * widthBytes;
        uint8_t* stripe = img + y * widthBytes;

        const int chunk = 180;

        for (int i = 0; i < stripeBytes; i += chunk) {
            int sendSize = min(chunk, stripeBytes - i);
            pRemoteChar->writeValue(stripe + i, sendSize, false);
            delay(3);
        }

        y += stripeHeight;
    }

    Serial.println("PRINT DONE");
}


void generateTestImage(uint8_t* img, int width, int height) {
    int widthBytes = width / 8;

    for (int y = 0; y < height; y++) {
        for (int xb = 0; xb < widthBytes; xb++) {
            uint8_t b = 0;
            for (int bit = 0; bit < 8; bit++) {
                if ((xb % 2) == 0) {
                    b |= (1 << (7 - bit));
                }
            }
            img[y * widthBytes + xb] = b;
        }
    }
}

void generateVerticalStripes(uint8_t* img, int width, int height) {
    int widthBytes = width / 8;
    int lineColorCounter  = 0;
    bool lineIsBlack = false;

    for (int y = 0; y < height; y++) {
        for (int xb = 0; xb < widthBytes; xb++) {
            uint8_t currentByte = 0;
            for (int bit = 0; bit < 8; bit++) {
                if (lineIsBlack) {
                    currentByte |= (1 << (7 - bit));
                }
            }
            img[y * widthBytes + xb] = currentByte;
        }
         lineColorCounter++;
         if (lineColorCounter >= 5) {
            lineIsBlack = !lineIsBlack;
            lineColorCounter = 0;
         }
    }
}


const int WIDTH = 96;
const int HEIGHT = 320;

uint8_t* img;

void testPrint() {
    int widthBytes = WIDTH / 8;
    img = (uint8_t*)malloc(widthBytes * HEIGHT);
    generateTestImage(img, WIDTH, HEIGHT);
   //  generateVerticalStripes(img, WIDTH, HEIGHT);
    printRasterPhomemo(img, WIDTH, HEIGHT);
    free(img);
}

void setup() {
   Serial.begin(115200);
   delay(2002);
   if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
   startScan();
}

unsigned long lastPing = 0;

void loop() {
   delay(888);
   counter++;
   if (pRemoteChar) {
      Serial.printf("Should be ready to print, counter: %d\n", counter);
      if (counter == 8) {
         Serial.println("   sending label...");
         // printLabelPolskafan2("HELLO");
         // printFileToQ30("/imageToPrintNew.png");
         // printStripes();
         // printPolskafanStripes2();
         // printPolskafanSync();
         // testPrint();
      }
      // if (millis() - lastPing > 60000) { // 10 minutes
      if (millis() - lastPing > 30000) { // 10 minutes
         Serial.println(">>> Sending keep-alive ping to printer...");
         uint8_t ping[] = {0x1f, 0x11, 0x07};
         pRemoteChar->writeValue(ping, sizeof(ping), false);
         lastPing = millis();
      }
   } else {
      Serial.println("Not connected to printer yet, trying again...");
      if (counter > 5) {
         connectToPrinter();
      }
   }
}
