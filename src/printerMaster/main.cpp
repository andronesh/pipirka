#include <NimBLEDevice.h>
#include <Adafruit_GFX.h>
#include <FS.h>
#include <LittleFS.h>

#include <U8g2_for_Adafruit_GFX.h>
#include "qrcode.h"

NimBLEClient* pClient;
NimBLERemoteCharacteristic* pRemoteChar;
NimBLERemoteCharacteristic* notifyChar;

int counter = 0;
int devicesFound = 0;

const char* PRINTER_MAC = "a2:ba:cb:2f:bd:e1";

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

void generateLabelImage(uint8_t* img, int width, int height, const char* text) {
   GFXcanvas1 canvas(width, height);

   canvas.fillScreen(0);
   canvas.setRotation(1);
   canvas.setCursor(96, 33);
   canvas.setTextColor(1);
   canvas.setTextSize(3);
   canvas.print(text);

   uint8_t* buf = canvas.getBuffer();
   memcpy(img, buf, width * height / 8);
}

void generateLabelWithQrCode(uint8_t* img, int width, int height, const char* text, const char* qrCodeText) {
   GFXcanvas1 canvas(width, height);

   canvas.fillScreen(0);
   canvas.setRotation(1);
   canvas.setCursor(100, 22);
   canvas.setTextColor(1);
   canvas.setTextSize(2);
   canvas.print(text);

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)]; // version 13, ECC_LOW?
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_MEDIUM, qrCodeText);

  int xOffset = 10;
  int yOffset = 5;
  int moduleSize = 2;

   for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
         if (qrcode_getModule(&qrcode, x, y)) {
         for (int mx = 0; mx < moduleSize; mx++)
            for (int my = 0; my < moduleSize; my++)
               canvas.drawPixel(xOffset + x * moduleSize + mx, yOffset + y * moduleSize + my, 1);
         }
      }
   }

   uint8_t* buf = canvas.getBuffer();
   memcpy(img, buf, width * height / 8);
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

// ===== Canvas setup =====
// #define LABEL_W 96
// #define LABEL_H 240
#define LABEL_W 240
#define LABEL_H 96
uint8_t canvas[LABEL_W * LABEL_H / 8]; // 1-bit per pixel
// Adafruit_GFX canvasGFX(LABEL_W, LABEL_H); // dummy GFX wrapper for U8g2
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// Dummy Adafruit_GFX wrapper
class MyCanvas : public Adafruit_GFX {
public:
  MyCanvas() : Adafruit_GFX(LABEL_W, LABEL_H) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (x < 0 || x >= LABEL_W || y < 0 || y >= LABEL_H) return;
    uint16_t byteIndex = x + y / 8 * LABEL_W;
    uint8_t bit = 1 << (y & 7);
    if (color) canvas[byteIndex] |= bit;
    else canvas[byteIndex] &= ~bit;
  }
};
MyCanvas myCanvas;

// ===== Helper functions =====

// Draw QR code at (x, y) with given module size
void drawQRCode(int x0, int y0, const char *text, int moduleSize = 2) {
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)]; // version 3, ECC_LOW
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, text);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        for (int mx = 0; mx < moduleSize; mx++)
          for (int my = 0; my < moduleSize; my++)
            myCanvas.drawPixel(x0 + x * moduleSize + mx, y0 + y * moduleSize + my, 1);
      }
    }
  }
}

// // Draw Code128 barcode at (x, y)
// void drawBarcode(int x0, int y0, const char *text, int barHeight = 40) {
//   Code128 barcode;
//   barcode.encode(text);

//   int xpos = x0;
//   for (int i = 0; i < barcode.length(); i++) {
//     if (barcode.get(i)) {
//       for (int h = 0; h < barHeight; h++)
//         myCanvas.drawPixel(xpos, y0 + h, 1);
//     }
//     xpos++;
//   }
// }

void rotate90CW(const uint8_t *src, uint8_t *dst, int w, int h) {
  memset(dst, 0, w*h/8); // clear destination

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      // read pixel from src
      int byteIndex = x + (y / 8) * w;
      int bit = 1 << (y & 7);
      bool pixelOn = src[byteIndex] & bit;

      if (pixelOn) {
        // write to dst rotated
        int newX = y;
        int newY = w - 1 - x;
        int dstByte = newX + (newY / 8) * h;
        int dstBit = 1 << (newY & 7);
        dst[dstByte] |= dstBit;
      }
    }
  }
}

const int WIDTH = 96;
const int HEIGHT = 240;

uint8_t* img;

void testPrint() {
    int widthBytes = WIDTH / 8;
    img = (uint8_t*)malloc(widthBytes * HEIGHT);
   //  generateTestImage(img, WIDTH, HEIGHT);
   //  generateVerticalStripes(img, WIDTH, HEIGHT);
    generateLabelWithQrCode(img, WIDTH, HEIGHT, "KABOOM 1881", "https://example.com");
    printRasterPhomemo(img, WIDTH, HEIGHT);
    free(img);
}

void testSuperPrint() {
      memset(canvas, 0, sizeof(canvas));

      u8g2Fonts.begin(myCanvas); // link U8g2 fonts to our canvas
      u8g2Fonts.setFont(u8g2_font_unifont_t_cyrillic);

      // Draw Cyrillic text
      u8g2Fonts.drawUTF8(0, 20, "Привіт, світ!");

      // Draw QR code
      // drawQRCode(0, 50, "https://example.com");

      // Draw barcode
      // drawBarcode(0, 150, "1234567890", 50);

      uint8_t rotatedCanvas[LABEL_W * LABEL_H / 8]; // same size, just rotated
      rotate90CW(canvas, rotatedCanvas, LABEL_H, LABEL_W);

      printRasterPhomemo(rotatedCanvas, LABEL_W, LABEL_H);
}

// void sendRotatedToPrinter(uint16_t w, uint16_t h, uint8_t* sourceBuffer) {
//     // New dimensions for the printer
//     uint16_t newW = h; 
//     uint16_t newH = w;
    
//     // Calculate bytes per row for the NEW (rotated) image
//     uint16_t newBytesPerRow = (newW + 7) / 8;
//     uint8_t* rotatedBuffer = (uint8_t*)calloc(newBytesPerRow * newH, 1);

//     if (rotatedBuffer == NULL) return; // Out of memory check

//     for (uint16_t y = 0; y < h; y++) {
//         for (uint16_t x = 0; x < w; x++) {
//             // Get pixel from U8g2 buffer
//             // U8g2 uses a specific tile-based or linear mapping depending on constructor
//             // u8g2.getBufferPtr() is generally linear for U8G2_BITMAP
//             bool pixel = u8g2.getPointer(x, y); // Helper to check pixel state

//             if (pixel) {
//                 // New coordinates for 90-degree rotation
//                 uint16_t newX = (h - 1 - y);
//                 uint16_t newY = x;

//                 // Set bit in the rotated buffer
//                 rotatedBuffer[newY * newBytesPerRow + (newX / 8)] |= (0x80 >> (newX % 8));
//             }
//         }
//     }

//     // --- PRINTER SENDING LOGIC ---
//     // Now send 'rotatedBuffer' to your printer using its bitmap command
//     // Example: myPrinter.printBitmap(newW, newH, rotatedBuffer);

//     printRasterPhomemo(rotatedBuffer, LABEL_W, LABEL_H);

//     free(rotatedBuffer); // Clean up memory!
// }

// U8G2_FOR_ADAFRUIT_GFX u8g2(WIDTH, HEIGHT, U8G2_R1);

// void testSimplePrint() {
  
// }

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
         testSuperPrint();
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
