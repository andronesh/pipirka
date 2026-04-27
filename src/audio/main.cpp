#include <Arduino.h>
#include "driver/i2s.h"
#include <math.h>

#define I2S_PORT I2S_NUM_0

#define I2S_BCLK 4
#define I2S_LRCK 5
#define I2S_DOUT 7

#define SAMPLE_RATE 16000

void setupI2S() {
    i2s_config_t config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false
    };

    i2s_pin_config_t pins = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_driver_install(I2S_PORT, &config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pins);
}

void playTone(float freq, int durationMs) {
    int samples = SAMPLE_RATE * durationMs / 1000;
    size_t bytesWritten;

    for (int i = 0; i < samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        int16_t sample = (int16_t)(sin(2 * PI * freq * t) * 10000); // volume

        i2s_write(I2S_PORT, &sample, sizeof(sample), &bytesWritten, portMAX_DELAY);
    }
}

void setup() {
    Serial.begin(115200);
    setupI2S();

    delay(1000);
    Serial.println("Playing test melody...");
}

void loop() {
    playTone(440, 300);  // A
    delay(100);

    playTone(523, 300);  // C
    delay(100);

    playTone(659, 300);  // E
    delay(300);
}