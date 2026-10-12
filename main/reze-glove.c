#include <stdio.h>
#include "led_strip.h"
#include "driver/gpio.h"

#define LED_GPIO 0

void app_main(void)
{
    // LED Config
    led_strip_handle_t led_strip = nullptr;
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = 60,
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

    while(true) {
        for(int i = 0; i < 20; ++i) {
            led_strip_set_pixel(led_strip, i, 255, i*5, 0);
            led_strip_refresh(led_strip);
        }
    }
}
