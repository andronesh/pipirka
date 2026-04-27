#include <Arduino.h>
#include "driver/i2s.h"

#define I2S_PORT I2S_NUM_0

// Pins
#define I2S_BCLK 4
#define I2S_LRCK 5
#define I2S_DIN  6   // mic
#define I2S_DOUT 7   // amp

#define BUTTON_PIN 9

// Audio settings
#define SAMPLE_RATE 16000
#define BUFFER_SAMPLES 16000 * 3  // 3 seconds

int16_t *audioBuffer;
size_t recordedSamples = 0;
bool recording = false;

void setupI2S() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false
    };

    i2s_pin_config_t pins = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_DIN
    };

    i2s_driver_install(I2S_PORT, &config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pins);
}

void setup() {
    Serial.begin(115200);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    audioBuffer = (int16_t*)malloc(BUFFER_SAMPLES * sizeof(int16_t));

    if (!audioBuffer) {
        Serial.println("Memory allocation failed!");
        while (1);
    }

    setupI2S();
}

void loop() {
    bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;

    if (buttonPressed && !recording) {
        Serial.println("Recording...");
        recording = true;
        recordedSamples = 0;
    }

    if (recording && buttonPressed) {
        size_t bytesRead;
        int16_t sample;

        if (recordedSamples < BUFFER_SAMPLES) {
            i2s_read(I2S_PORT, &sample, sizeof(sample), &bytesRead, portMAX_DELAY);
            audioBuffer[recordedSamples++] = sample;
        }
    }

    if (!buttonPressed && recording) {
        Serial.println("Playback...");
        recording = false;

        size_t bytesWritten;

        for (size_t i = 0; i < recordedSamples; i++) {
            i2s_write(I2S_PORT, &audioBuffer[i], sizeof(int16_t), &bytesWritten, portMAX_DELAY);
        }

        i2s_zero_dma_buffer(I2S_PORT);
        i2s_stop(I2S_PORT);
        delay(10);
        i2s_start(I2S_PORT);

        Serial.println("Done");
    }
}