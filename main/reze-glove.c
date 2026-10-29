#include <stdio.h>
#include "led_strip.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "NEOPIXEL";

#define LED_GPIO 0
#define BUTTON_GPIO GPIO_NUM_10
#define LED_COUNT 100

#define PRESS_MS 500
#define DEBOUNCE_MS 100

TaskHandle_t mainTaskHandle = nullptr;
volatile TickType_t lastEdge = 0;


static void IRAM_ATTR button_isr(void *arg) {
    TickType_t now = xTaskGetTickCountFromISR();
    if((now - lastEdge < pdMS_TO_TICKS(DEBOUNCE_MS))) {
        return;
    }
    if (gpio_get_level(BUTTON_GPIO) != 0) {
        return;
    }
    lastEdge = now;

    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(mainTaskHandle, 1, eSetValueWithOverwrite, &higherPriorityTaskWoken);
    if (higherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void set_color(led_strip_handle_t led_strip, uint32_t color, const uint32_t led_nums[], size_t count) {
    uint8_t red   = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8)  & 0xFF;
    uint8_t blue  = color & 0xFF;

    for(size_t i = 0; i < count; ++i){
        led_strip_set_pixel(led_strip, led_nums[i], red, green, blue);
    }

    led_strip_refresh(led_strip);
}

void fade_into(led_strip_handle_t led_strip, uint32_t color, const uint32_t led_nums[], size_t count) {
    uint8_t red   = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8)  & 0xFF;
    uint8_t blue  = color & 0xFF;

    for(int i = 0; i <= 50; i++) {
        for(size_t j = 0; j < count; j++){
            led_strip_set_pixel(led_strip, led_nums[j], red*i/50, green*i/50, blue*i/50);
        }
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
}

void app_main(void)
{
    mainTaskHandle = xTaskGetCurrentTaskHandle();
    // LED Config
    led_strip_handle_t led_strip = nullptr;
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false
        }
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = false
        }
    };
    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(
            &strip_config,
            &rmt_config,
            &led_strip
        )
    );
    
    led_strip_clear(led_strip);

    gpio_reset_pin(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, nullptr);

    uint32_t my_array[] = {42, 43, 44, 49, 50, 51, 52, 53, 59, 60, 61, 62, 68, 69, 70, 71, 72, 77, 78, 79, 80, 81, 84, 85, 86, 87, 88};
    size_t array_count = sizeof(my_array) / sizeof(my_array[0]);

    while(true) {
        led_strip_clear(led_strip);
        uint32_t clicks = 0;
        uint32_t pressEvent = 0;
        xTaskNotifyWait(0, UINT32_MAX, &pressEvent, portMAX_DELAY); //first wake
        
        while(true) {
            BaseType_t result = xTaskNotifyWait(0, UINT32_MAX, &pressEvent, pdMS_TO_TICKS(PRESS_MS));
            ESP_LOGI(TAG, "IN PRESS......");
            if(result == pdFALSE) {
                break;
            }
            clicks++;

        }

        ESP_LOGI(TAG, "Click count: %lu",
         (unsigned long)clicks);

        if(clicks == 0) { //ONE PRESS -- Fingers reaching out animation
            ESP_LOGI(TAG, "First press");
            fade_into(led_strip, 0xFF1900, my_array, array_count);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        else if(clicks == 1) { //TWO PRESS -- On mode for photos
            ESP_LOGI(TAG, "Second press");
            for(int i = 0; i < 15; ++i){
                uint32_t my_array1[] = {(i * 12) % 100};
                uint32_t my_array2[] = {(i * 4) % 100};
                uint32_t my_array3[] = {(i * 24) % 100};
                set_color(led_strip, 0xFF1900, my_array1, 1);
                set_color(led_strip, 0xFF2000, my_array3, 1);
                set_color(led_strip, 0x000000, my_array2, 1);
            }
            xTaskNotifyWait(0, UINT32_MAX, &pressEvent, portMAX_DELAY);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        uint32_t ignored;
        while (xTaskNotifyWait(0, UINT32_MAX, &ignored, 0) == pdTRUE) {}
    }
}
