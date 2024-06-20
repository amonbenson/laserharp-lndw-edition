#pragma once

#include "sensor.h"


typedef struct {
    int size;
    const int *sensor_index_lut;
    int calibration_iterations;
} harp_config_t;

typedef struct {
    int x;
    int y;
    bool active;
    uint8_t note;
} harp_state_t;

typedef struct {
    harp_config_t config;
    sensor_t sensor;

    harp_state_t current;
    harp_state_t previous;
    harp_state_t prevraw;
    bool flipped;
} harp_t;


esp_err_t harp_init(harp_t *harp, const harp_config_t *config);

esp_err_t harp_update(harp_t *harp);
