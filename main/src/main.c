#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_check.h>
#include "sensor.h"


#define SENSOR_SAMPLE_PERIOD 20

static const char *TAG = "main";


void sensor_task(void *arg) {
    sensor_t sensor;

    ESP_ERROR_CHECK(sensor_init(&sensor));

    vTaskDelay(1000 / portTICK_RATE_MS);
    ESP_ERROR_CHECK(sensor_calibrate(&sensor));

    while (1) {
        ESP_ERROR_CHECK(sensor_update(&sensor));

        /* ESP_LOGI(TAG, "value : %d, state: %d, offset: %d, sum: %d, calibration_count: %d",
            sensor.ldrs_left[0].value, sensor.ldrs_left[0].state,
            sensor.ldrs_left[0].calibration.offset, sensor.ldrs_left[0].calibration.sum,
            sensor.calibration_count); */
        for (int i = 0; i < SENSOR_ADC_NUM_CHANNELS; i++) {
            printf("%04d%c ", sensor.ldrs_left[i].value, sensor.ldrs_left[i].state ? 'X' : ' ');
        }
        printf("| ");
        for (int i = 0; i < SENSOR_ADC_NUM_CHANNELS; i++) {
            printf("%04d%c ", sensor.ldrs_right[i].value, sensor.ldrs_right[i].state ? 'X' : ' ');
        }
        printf("\n");

        vTaskDelay(SENSOR_SAMPLE_PERIOD / portTICK_RATE_MS);
    }
}

void app_main(void) {
    TaskHandle_t sensor_task_handle;

    // start the sensor reading task
    xTaskCreatePinnedToCore(
        sensor_task,
        "sensor_task",
        4096,
        NULL,
        5,
        &sensor_task_handle,
        0);
    assert(sensor_task_handle);

    /* const gpio_config_t config = {
        .pin_bit_mask = 0xffff << 1,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE // GPIO_INTR_ANYEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&config));

    while (1) {
        for (int i = 0; i < 16; i++) {
            printf("%c ", gpio_get_level(i + 1) ? 'O' : '.');
        }
        printf("\n");

        vTaskDelay(SENSOR_SAMPLE_PERIOD / portTICK_RATE_MS);
    } */
}
