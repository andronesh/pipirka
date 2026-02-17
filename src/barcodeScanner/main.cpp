#include <Arduino.h>
#include "usb/usb_host.h"
#include "esp_log.h"

// S3 USB Host Pins
#define USB_DP_PIN 44
#define USB_DM_PIN 43

// This task handles the USB controller events
void usb_host_task(void *pvParameters) {
    while (1) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("--- ESP32-S3 USB Barcode Reader ---");

    // 1. Install USB Host Library
    usb_host_config_t host_config = {
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    
    esp_err_t err = usb_host_install(&host_config);
    if (err == ESP_OK) {
        Serial.println("USB Host Installed Successfully");
    } else {
        Serial.printf("USB Host Install Failed: %s\n", esp_err_to_name(err));
        return;
    }

    // 2. Create a background task to handle USB events
    xTaskCreatePinnedToCore(usb_host_task, "usb_host", 4096, NULL, 2, NULL, 0);

    Serial.println("Waiting for 1610S Scanner...");
}

void loop() {
    // The library handles the data in the background task.
    // You can add your business logic here!
    delay(10);
}
