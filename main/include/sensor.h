#pragma once

#include <driver/adc.h>
#include <esp_err.h>


#define SENSOR_CALIBRATION_DONE -1


typedef struct {
    int num_channels;
    const int *index_lut;
    struct {
        adc_bits_width_t width;
        adc_atten_t atten;
    } adc;
    struct {
        int iterations;
        int threshold;
    } calibration;
} sensor_config_t;

typedef struct {
    int value;
    struct {
        int offset;
        int sum;
    } calibration;
} sensor_ldr_t;

typedef uint16_t sensor_state_t;

typedef struct {
    sensor_config_t config;
    sensor_ldr_t *ldrs;
    struct {
        sensor_state_t current;
        sensor_state_t previous;
        sensor_state_t pressed;
        sensor_state_t released;
    } state;
    struct {
        int counter;
    } calibration;
} sensor_t;


esp_err_t sensor_init(sensor_t *sensor, const sensor_config_t *config);
esp_err_t sensor_destroy(sensor_t *sensor);

esp_err_t sensor_update(sensor_t *sensor);

esp_err_t sensor_calibrate(sensor_t *sensor);
