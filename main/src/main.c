#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_check.h>
#include "harp.h"


#define HARP_SIZE 8
#define SENSOR_SAMPLE_PERIOD 20

static const char *TAG = "main";

static const int sensor_index_lut[HARP_SIZE * 2] = {
    15, 12, 3, 6, 0, 5, 10, 4,
    7, 1, 2, 11, 13, 14, 9, 8
};


int app_main(void) {
    harp_t harp;

    const gpio_config_t gpio_input_config = {
        .pin_bit_mask = (1 << 9),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_input_config),
        TAG, "Failed to configure calibrate gpio");

    const harp_config_t harp_config = {
        .size = HARP_SIZE,
        .sensor_index_lut = (const int *) sensor_index_lut,
        .calibration_iterations = 100
    };
    ESP_ERROR_CHECK(harp_init(&harp, &harp_config));

    while (1) {
        int cal = gpio_get_level(9);
        if (cal == 0) {
            sensor_calibrate(&harp.sensor);
        }

        ESP_ERROR_CHECK(harp_update(&harp));
        vTaskDelay(SENSOR_SAMPLE_PERIOD / portTICK_PERIOD_MS);
    }

    return 0;
}
