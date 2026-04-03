#include <lvgl.h>
#include <TFT_eSPI.h>
#include "Button.h"

TFT_eSPI tft = TFT_eSPI();

static lv_color_t buf[240 * 20]; // partial buffer

const int PIN_TFT_BL = 1;
#define PWM_CH 0
unsigned long previousMillis = 0;
const long interval = 3000;
int screenBrightness = 0;

lv_obj_t *brightnessLabel;

const gpio_num_t PIN_BTN_UP = GPIO_NUM_2;
const gpio_num_t PIN_BTN_DOWN = GPIO_NUM_3;

void applyBrightness() {
    ledcWrite(PWM_CH, screenBrightness);

    // lv_label_set_text_fmt(brightnessLabel, "BL: %d", screenBrightness);
    lv_label_set_text(brightnessLabel,  std::to_string(screenBrightness).c_str());
    // lv_obj_invalidate(brightnessLabel);
    Serial.printf("--- brightness: %d\n", screenBrightness);
}

static void onUpButtonPressDownCb(void *button_handle, void *usr_data) {
    Serial.println("--- press down UP button");
    screenBrightness += 15;
    if (screenBrightness > 255) screenBrightness = 255;
    applyBrightness();
}

static void onKeyDownButtonPressDownCb(void *button_handle, void *usr_data) {
    Serial.println("--- press down DOWN button");
    screenBrightness -= 15;
    if (screenBrightness < 0) screenBrightness = 0;
    applyBrightness();
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

void drawInitialUI() {
    lv_obj_t *greetingsLabel = lv_label_create(lv_screen_active());
    lv_label_set_text(greetingsLabel, "Hello LVGL 9.5.0!");
    lv_obj_set_style_text_font(greetingsLabel, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(greetingsLabel, lv_color_white(), 0);
    lv_obj_center(greetingsLabel);

    brightnessLabel = lv_label_create(lv_screen_active());
    lv_label_set_text(brightnessLabel,  std::to_string(screenBrightness).c_str());
    lv_obj_set_style_text_font(brightnessLabel, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(brightnessLabel, lv_color_white(), 0);

    // place BELOW first label
    lv_obj_align_to(brightnessLabel, greetingsLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void setup() {
    Serial.begin(115200);

    Button *btnUp = new Button(PIN_BTN_UP, false);
    btnUp->attachPressDownEventCb(&onUpButtonPressDownCb, NULL);

    Button *btnDown = new Button(PIN_BTN_DOWN, false);
    btnDown->attachPressDownEventCb(&onKeyDownButtonPressDownCb, NULL);


    tft.init();
    tft.invertDisplay(false);
    tft.setRotation(1);

    ledcSetup(PWM_CH, 5000, 8);   // channel, freq, resolution
    ledcAttachPin(PIN_TFT_BL, PWM_CH);

    ledcWrite(PWM_CH, screenBrightness); // max brightness

    lv_init();

    // Create display
    lv_display_t *disp = lv_display_create(240, 240);

    // Set buffers (NEW API)
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set flush callback
    lv_display_set_flush_cb(disp, my_disp_flush);

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);

    drawInitialUI();
}

void loop() {
    lv_timer_handler();
    delay(5);
}