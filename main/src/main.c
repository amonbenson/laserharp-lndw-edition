#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_check.h>
#include "harp.h"


#define HARP_SIZE 8
#define SENSOR_SAMPLE_PERIOD 100

static const char *TAG = "main";

static const int sensor_index_lut[HARP_SIZE * 2] = {
    15, 12, 3, 6, 0, 5, 10, 4,
    7, 1, 2, 11, 13, 14, 9, 8
};


void app_main(void) {
    harp_t harp;

    const harp_config_t harp_config = {
        .size = HARP_SIZE,
        .sensor_index_lut = (const int *) sensor_index_lut,
        .calibration_iterations = 1000 / SENSOR_SAMPLE_PERIOD
    };
    ESP_ERROR_CHECK(harp_init(&harp, &harp_config));

    while (1) {
        ESP_ERROR_CHECK(harp_update(&harp));
        vTaskDelay(SENSOR_SAMPLE_PERIOD / portTICK_RATE_MS);
    }
}
