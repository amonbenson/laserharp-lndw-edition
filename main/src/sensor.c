#include "sensor.h"
#include <stdlib.h>
#include <string.h>
#include <driver/gpio.h>
#include <esp_check.h>


static const char *TAG = "sensor";


esp_err_t sensor_init(sensor_t *sensor, const sensor_config_t *config) {
    // init the sensor struct
    sensor->config = *config;
    sensor->calibration.counter = -1;
    sensor->state.current = 0;
    sensor->state.previous = 0;
    sensor->state.pressed = 0;
    sensor->state.released = 0;

    sensor->ldrs = calloc(config->num_channels * 2, sizeof(sensor_ldr_t));
    ESP_RETURN_ON_FALSE(sensor->ldrs, ESP_ERR_NO_MEM,
        TAG, "failed to allocate memory for ldrs");

    // configure all adc gpios as inputs
    gpio_num_t dac1_gpio, dac2_gpio;
    adc1_pad_get_io_num(0, &dac1_gpio);
    adc2_pad_get_io_num(0, &dac2_gpio);
    uint64_t bitmask = 0xff << dac1_gpio | 0xff << dac2_gpio;

    const gpio_config_t gpio_input_config = {
        .pin_bit_mask = bitmask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_input_config),
        TAG, "Failed to configure adc gpios");

    // init adc1
    ESP_LOGI(TAG, "initializing adc1");

    ESP_RETURN_ON_ERROR(adc1_config_width(sensor->config.adc.width),
        TAG, "adc1: failed to configure width");

    for (int i = 0; i < sensor->config.num_channels; i++) {
        ESP_RETURN_ON_ERROR(adc1_config_channel_atten(i, sensor->config.adc.atten),
            TAG, "adc1 channel %d: failed to configure attenuation", i);
    }

    // init adc2 (width is configured per reading)
    ESP_LOGI(TAG, "initializing adc2");

    for (int i = 0; i < sensor->config.num_channels; i++) {
        ESP_RETURN_ON_ERROR(adc2_config_channel_atten(i, sensor->config.adc.atten),
            TAG, "adc2 channel %d: failed to configure attenuation", i);
    }

    return ESP_OK;
}

esp_err_t sensor_destroy(sensor_t *sensor) {
    // TODO: implement
    return ESP_OK;
}

static bool sensor_update_ldr(sensor_t *sensor, sensor_ldr_t *ldr, int value) {
    // update the ldr's calibration parameters
    if (sensor->calibration.counter > 0) {
        ldr->calibration.sum += value;
    } else if (sensor->calibration.counter == 0) {
        ldr->calibration.offset = ldr->calibration.sum / sensor->config.calibration.iterations;
    }

    // set the new raw value and return the state
    ldr->value = value;
    return value - ldr->calibration.offset >= sensor->config.calibration.threshold;
}

esp_err_t sensor_update(sensor_t *sensor) {
    int index, v;
    bool s;

    sensor->state.previous = sensor->state.current;
    sensor->state.current = 0;
    sensor->state.pressed = 0;
    sensor->state.released = 0;

    // read adc 1
    for (int i = 0; i < sensor->config.num_channels; i++) {
        index = sensor->config.index_lut[i];
        v = adc1_get_raw(i);
        s = sensor_update_ldr(sensor, &sensor->ldrs[index], v);
        sensor->state.current |= s << index;
    }

    // read adc 2
    for (int i = 0; i < sensor->config.num_channels; i++) {
        index = sensor->config.index_lut[i + sensor->config.num_channels];
        ESP_RETURN_ON_ERROR(adc2_get_raw(i, sensor->config.adc.width, &v),
            TAG, "adc2 channel %d: failed to read", i);
        s = sensor_update_ldr(sensor, &sensor->ldrs[index], v);
        sensor->state.current |= s << index;
    }

    // update the state event flags
    if (sensor->calibration.counter == SENSOR_CALIBRATION_DONE) {
        sensor->state.pressed = sensor->state.current & ~sensor->state.previous;
        sensor->state.released = sensor->state.previous & ~sensor->state.current;
    }

    // update the calibration counter
    if (sensor->calibration.counter > 0) {
        sensor->calibration.counter--;
    } else if (sensor->calibration.counter == 0) {
        sensor->calibration.counter = SENSOR_CALIBRATION_DONE;
        ESP_LOGI(TAG, "calibration done");
    }

    return ESP_OK;
}

esp_err_t sensor_calibrate(sensor_t *sensor) {
    sensor->calibration.counter = sensor->config.calibration.iterations;

    for (int i = 0; i < sensor->config.num_channels * 2; i++) {
        sensor->ldrs[i].calibration.sum = 0;
    }

    ESP_LOGI(TAG, "calibrating...");
    return ESP_OK;
}
