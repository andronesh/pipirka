#include <NimBLEDevice.h>
#include <Adafruit_GFX.h>
#include <FS.h>
#include <LittleFS.h>

#include "qrcode.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

const char* ssid = "";
const char* password = "";

AsyncWebServer server(80);

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

void printRasterPhomemo(uint8_t* img, int width, int height) {
    if (!pRemoteChar) return;
    Serial.println(">>> Starting raster print...");

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

GFXcanvas1 initCanvas(int width, int height) {
   GFXcanvas1 canvas(width, height);

   canvas.fillScreen(0);
   canvas.setRotation(1);
   return canvas;
}

void drawText(GFXcanvas1& canvas, int x, int y, const char* text, int size) {
   canvas.setCursor(x, y);
   canvas.setTextColor(1);
   canvas.setTextSize(size);
   canvas.print(text);
}

void drawQR(GFXcanvas1& canvas, int offX, int offY, const char* text, int size = 3, int ECC = 2, int cellSize = 2) {
   QRCode qrcode;
   uint8_t qrcodeData[qrcode_getBufferSize(size)];
   qrcode_initText(&qrcode, qrcodeData, size, ECC, text);

   for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
         if (qrcode_getModule(&qrcode, x, y)) {
         for (int mx = 0; mx < cellSize; mx++)
            for (int my = 0; my < cellSize; my++)
               canvas.drawPixel(offX + x * cellSize + mx, offY + y * cellSize + my, 1);
         }
      }
   }
}

void drawTestImage(GFXcanvas1& canvas) {
   drawText(canvas, 111, 20, "Test Image", 2);
   drawText(canvas, 111, 50, "1234567890", 3);
   drawQR(canvas, 13, 7, "1234567890", 3, ECC_MEDIUM, 3);
}

void testGenerablePrint() {
   const int WIDTH = 96;
   const int HEIGHT = 320;
   const int widthBytes = WIDTH / 8;
   uint8_t* imgBuf = (uint8_t*)malloc(widthBytes * HEIGHT);

   GFXcanvas1 canvas = initCanvas(WIDTH, HEIGHT);
   drawTestImage(canvas);
   Serial.println("   Finished drawing on canvas, sending to printer...");
   uint8_t* buf = canvas.getBuffer();
   memcpy(imgBuf, buf, widthBytes * HEIGHT);
   printRasterPhomemo(imgBuf, WIDTH, HEIGHT);
   free(imgBuf);
   Serial.println("   Print command sent to printer, buffer freed.");
}

void connectToWifi() {
   if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Already connected to WiFi.");
      return;
   }
   Serial.printf("Connecting to WiFi: %s\n", ssid);
   WiFi.begin(ssid, password);
   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
   }
   Serial.printf("\nWiFi connected! IP address: %s\n", WiFi.localIP().toString().c_str());
}

void startupServer() {
   server.on("/label", HTTP_POST,
      [](AsyncWebServerRequest *request){},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
      {
         Serial.println(">>> /label endpoint hit, processing data...");
         StaticJsonDocument<2048> doc;
         DeserializationError err = deserializeJson(doc, data);
         if (err) {
            request->send(400, "application/json", "{\"status\":\"json_error\"}");
            return;
         }

         const int WIDTH = 96;
         // const int HEIGHT = 240;
         const int HEIGHT = 320;
         const int widthBytes = WIDTH / 8;
         uint8_t* imgBuf = (uint8_t*)malloc(widthBytes * HEIGHT);

         GFXcanvas1 canvas = initCanvas(WIDTH, HEIGHT);

         JsonArray elements = doc["elements"];

         Serial.printf("   Elements to print: %d\n", elements.size());

         for (JsonObject element : elements) {
            const char* type = element["type"];

            const int x = element["x"] | 0;
            const int y = element["y"] | 0;
            const char* value = element["value"];

            if (strcmp(type, "text") == 0) {
               Serial.printf("   Drawing text at (%d, %d), value: '%s'\n", x, y, value);
               int size = element["size"] | 1;

               drawText(canvas, x, y, value, size);
            }
            else if (strcmp(type, "qr") == 0) {
               Serial.printf("   Drawing QR code at (%d, %d), value: '%s'\n", x, y, value);
               int size = element["size"] | 3;
               int ECC = element["ecc"] | 2;
               int cellSize = element["cellSize"] | 2;

               drawQR(canvas, x, y, value, size, ECC, cellSize);
            }
         }
         Serial.println("   Finished drawing on canvas, sending to printer...");
         uint8_t* buf = canvas.getBuffer();
         memcpy(imgBuf, buf, widthBytes * HEIGHT);
         printRasterPhomemo(imgBuf, WIDTH, HEIGHT);
         free(imgBuf);
         Serial.println("   Print command sent to printer, responding OK to client.");
         request->send(200, "application/json", "{\"status\":\"ok\"}");
      }
   );

   server.begin();
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
         startupServer();
      }
      if (millis() - lastPing > 30000) { // 60000 is 10 minutes
         Serial.println(">>> Sending keep-alive ping to printer...");
         uint8_t ping[] = {0x1f, 0x11, 0x07};
         pRemoteChar->writeValue(ping, sizeof(ping), false);
         lastPing = millis();
      }
   } else {
      Serial.println("Not connected to printer yet, trying again...");
      if (counter > 5) {
         connectToWifi();
         connectToPrinter();
      }
   }
}


/*
POST /label
Content-Type: application/json

{
  "elements": [
    { "type":"text", "x":108, "y":20, "value":"BESHKET 13", "size":2 },
    { "type":"text", "x":108, "y":50, "value":"12498", "size":4 },
    { "type":"qr", "x":13, "y":7, "value":"http://example.com", "size": 3, "ECC": 2, "cellSize": 3 }
  ]
}

*/