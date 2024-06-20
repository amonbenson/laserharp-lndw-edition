#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_check.h>
#include "harp.h"


#define HARP_SIZE 8
#define SENSOR_SAMPLE_PERIOD 20

#define CALIBRATE_PIN 9
#define FLIP_PIN 40

static const char *TAG = "main";

static const int sensor_index_lut[HARP_SIZE * 2] = {
    15, 12, 3, 6, 0, 5, 10, 4,
    7, 1, 2, 11, 13, 14, 9, 8
};


int app_main(void) {
    /* while (1) {
        for (int i = 0; i < 46; i++) {
            gpio_set_direction(i, GPIO_MODE_INPUT);
            gpio_set_pull_mode(i, GPIO_PULLUP_ONLY);
            int level = gpio_get_level(i);
            printf("%i: %d ", i, level);
        }
        printf("\n");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } */

    harp_t harp;

    // release all notes
    for (int note = 0; note <= 127; note++) {
        putchar(0x90);
        putchar(note);
        putchar(0);
    }

    gpio_set_direction(CALIBRATE_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(CALIBRATE_PIN, GPIO_PULLUP_ONLY);

    gpio_set_direction(FLIP_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(FLIP_PIN, GPIO_PULLUP_ONLY);

    const harp_config_t harp_config = {
        .size = HARP_SIZE,
        .sensor_index_lut = (const int *) sensor_index_lut,
        .calibration_iterations = 100
    };
    ESP_ERROR_CHECK(harp_init(&harp, &harp_config));

    int prev_flip = 1;

    while (1) {
        int cal = gpio_get_level(CALIBRATE_PIN);
        if (cal == 0) {
            sensor_calibrate(&harp.sensor);

            // release all notes
            for (int note = 0; note <= 127; note++) {
                putchar(0x90);
                putchar(note);
                putchar(0);
            }
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }

        int flip = gpio_get_level(FLIP_PIN);
        if (flip == 0 && prev_flip == 1) {
            harp_flip(&harp);
        }
        prev_flip = flip;

        ESP_ERROR_CHECK(harp_update(&harp));
        vTaskDelay(SENSOR_SAMPLE_PERIOD / portTICK_PERIOD_MS);
    }

    return 0;
}
