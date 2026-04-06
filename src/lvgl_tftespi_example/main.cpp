#include <lvgl.h>
#include <TFT_eSPI.h>
#include "Button.h"

TFT_eSPI tft = TFT_eSPI();
static lv_color_t buf[240 * 20]; // partial buffer

int counter = 0;
lv_obj_t *counterLabel;

const gpio_num_t PIN_BTN_UP = GPIO_NUM_1;
const gpio_num_t PIN_BTN_DOWN = GPIO_NUM_3;

void updateCounterLabel() {
    // lv_label_set_text_fmt(counterLabel, "Count: %d", counter);
    lv_label_set_text(counterLabel, std::to_string(counter).c_str());
    // lv_obj_invalidate(counterLabel);
    Serial.printf("--- counter should be %d\n", counter);
}

static void onUpButtonPressDownCb(void *button_handle, void *usr_data) {
    Serial.println("--- press down UP button");
    counter += 15;
    updateCounterLabel();
}

static void onKeyDownButtonPressDownCb(void *button_handle, void *usr_data) {
    Serial.println("--- press down DOWN button");
    counter -= 15;
    updateCounterLabel();
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

    counterLabel = lv_label_create(lv_screen_active());
    lv_label_set_text(counterLabel, std::to_string(counter).c_str());
    lv_obj_set_style_text_font(counterLabel, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(counterLabel, lv_color_white(), 0);

    lv_obj_align_to(counterLabel, greetingsLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void setup() {
    Serial.begin(115200);

    Button *btnUp = new Button(PIN_BTN_UP, false);
    btnUp->attachPressDownEventCb(&onUpButtonPressDownCb, NULL);

    Button *btnDown = new Button(PIN_BTN_DOWN, false);
    btnDown->attachPressDownEventCb(&onKeyDownButtonPressDownCb, NULL);

    tft.init();
    tft.invertDisplay(false);
    tft.setRotation(3);

    lv_init();

    lv_display_t *disp = lv_display_create(240, 240);
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, my_disp_flush);

    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);

    drawInitialUI();
}

void loop() {
    lv_timer_handler();
    delay(5);
}