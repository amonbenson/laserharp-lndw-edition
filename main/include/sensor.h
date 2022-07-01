#pragma once

#include <driver/adc.h>
#include <esp_err.h>


#define SENSOR_ADC_WIDTH ADC_WIDTH_BIT_12
#define SENSOR_ADC_NUM_CHANNELS 8

#define SENSOR_CALIBRATION_ITERATIONS 32
#define SENSOR_THRESHOLD 100


typedef struct {
    int value;
    bool state;

    struct {
        int offset;
        int sum;
    } calibration;
} sensor_ldr_t;

typedef struct {
    sensor_ldr_t ldrs_left[SENSOR_ADC_NUM_CHANNELS];
    sensor_ldr_t ldrs_right[SENSOR_ADC_NUM_CHANNELS];
    int calibration_count;
} sensor_t;


esp_err_t sensor_init(sensor_t *sensor);

esp_err_t sensor_update(sensor_t *sensor);

esp_err_t sensor_calibrate(sensor_t *sensor);
