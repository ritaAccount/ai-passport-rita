// main/main.c —— 开机进入老虎机（SLOTS）；OK 长按重置筹码与等级进度。
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "bsp_pins.h"
#include "demo.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char *TAG = "main";

static bool s_btn_ok;
static bool s_batt_ok;

static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    if (!bsp_lvgl_lock(500)) return;

    if (btn == BSP_BTN_OK && ev == BSP_BTN_LONG) {
        demo_slots_reset_progress();
    } else {
        demo_slots_key(btn, ev);
    }

    bsp_lvgl_unlock();
}

void app_main(void)
{
    ESP_LOGI(TAG, "FoloToy AI Passport SLOTS 启动");
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup != ESP_SLEEP_WAKEUP_UNDEFINED) {
        ESP_LOGI(TAG, "休眠唤醒原因: %d", wakeup);
    }

    bsp_i2c_init();
    bsp_i2c_scan();

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败。"
                      "检查 SPI(MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    s_btn_ok = (bsp_button_init(on_key, NULL) == ESP_OK);
    s_batt_ok = (bsp_battery_init() == ESP_OK);

    if (bsp_lvgl_lock(1000)) {
        demo_slots_enter();
        bsp_lvgl_unlock();
    }

    ESP_LOGI(TAG, "SLOTS 就绪 Button=%d Battery=%d", s_btn_ok, s_batt_ok);
}
