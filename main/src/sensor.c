#include "sensor.h"
#include <string.h>
#include <esp_check.h>


static const char *TAG = "sensor";


esp_err_t sensor_init(sensor_t *sensor) {
    // init the sensor struct
    memset(sensor, 0, sizeof(sensor_t));
    sensor->calibration_count = -1;

    // init adc1
    ESP_LOGI(TAG, "initializing adc1");

    ESP_RETURN_ON_ERROR(adc1_config_width(SENSOR_ADC_WIDTH),
        TAG, "adc1: failed to configure width");

    for (int i = 0; i < SENSOR_ADC_NUM_CHANNELS; i++) {
        ESP_RETURN_ON_ERROR(adc1_config_channel_atten(i, ADC_ATTEN_DB_11),
            TAG, "adc1 channel %d: failed to configure attenuation", i);
    }

    // init adc2 (width is configured per reading)
    ESP_LOGI(TAG, "initializing adc2");

    for (int i = 0; i < SENSOR_ADC_NUM_CHANNELS; i++) {
        ESP_RETURN_ON_ERROR(adc2_config_channel_atten(i, ADC_ATTEN_DB_11),
            TAG, "adc2 channel %d: failed to configure attenuation", i);
    }

    return ESP_OK;
}

static void sensor_update_ldr(sensor_t *sensor, sensor_ldr_t *ldr, int value) {
    // update the ldr's calibration parameters
    if (sensor->calibration_count > 0) {
        ldr->calibration.sum += value;
    } else if (sensor->calibration_count == 0) {
        ldr->calibration.offset = ldr->calibration.sum / SENSOR_CALIBRATION_ITERATIONS;
    }

    // set the new raw value and state
    ldr->value = value;
    ldr->state = value - ldr->calibration.offset >= SENSOR_THRESHOLD;
}

esp_err_t sensor_update(sensor_t *sensor) {
    int v;

    // read left channels
    for (int i = 0; i < SENSOR_ADC_NUM_CHANNELS; i++) {
        v = adc1_get_raw(i);
        sensor_update_ldr(sensor, &sensor->ldrs_left[i], v);
    }

    // read right channels
    for (int i = 0; i < SENSOR_ADC_NUM_CHANNELS; i++) {
        ESP_RETURN_ON_ERROR(adc2_get_raw(i, SENSOR_ADC_WIDTH, &v),
            TAG, "adc2 channel %d: failed to read", i);
        sensor_update_ldr(sensor, &sensor->ldrs_right[i], v);
    }

    // update the calibration counter
    if (sensor->calibration_count > -1) sensor->calibration_count--;

    return ESP_OK;
}

esp_err_t sensor_calibrate(sensor_t *sensor) {
    sensor->calibration_count = SENSOR_CALIBRATION_ITERATIONS;

    for (int i = 0; i < SENSOR_ADC_NUM_CHANNELS; i++) {
        sensor->ldrs_left[i].calibration.sum = 0;
        sensor->ldrs_right[i].calibration.sum = 0;
    }

    return ESP_OK;
}
