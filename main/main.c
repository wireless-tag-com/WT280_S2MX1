#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "soc/soc_caps.h"
#include "rom/gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"
#include "i2c_device.h"
#include "touch_panel.h"
#include "screen_driver.h"
#include "board.h"


#define TAG "MAIN"

static lv_disp_drv_t disp_drv;     
static scr_driver_t g_lcd;

static void increase_lvgl_tick(void* arg) {
    lv_tick_inc(portTICK_PERIOD_MS);
}

static void lvgl_drv_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    g_lcd.draw_bitmap(area->x1, area->y1, (uint16_t)(area->x2 - area->x1 + 1), (uint16_t)(area->y2 - area->y1 + 1), (uint16_t *)color_map);
    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(drv);
}

void lvgl_task(void* arg) {
    lv_init();

    static lv_disp_draw_buf_t draw_buf;

    lv_color_t *buf1 = NULL;
    lv_color_t *buf2 = NULL;
    buf1 = heap_caps_aligned_calloc(64, 1, LCD_WIDTH * 40 * 2, MALLOC_CAP_DMA);
    //buf2 = heap_caps_aligned_calloc(64, 1, LCD_WIDTH * LCD_HIGHT * 2, MALLOC_CAP_SPIRAM);
    
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_WIDTH * LCD_HIGHT);

    lv_disp_drv_init(&disp_drv);         
    disp_drv.flush_cb = lvgl_drv_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HIGHT;
    lv_disp_drv_register(&disp_drv);

    // Tick interface for LVGL
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = increase_lvgl_tick,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, portTICK_PERIOD_MS * 1000);

    extern void lv_demo_widgets(void);
    lv_demo_widgets();

    gpio_set_level(LCD_BL_PIN, 1);

    for (;;) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    extern void screen_init(scr_driver_t* s_lcd, SemaphoreHandle_t semaphore);
    screen_init(&g_lcd, NULL);

    xTaskCreatePinnedToCore(lvgl_task, NULL, 8 * 1024, NULL, 5, NULL, 1);
}
