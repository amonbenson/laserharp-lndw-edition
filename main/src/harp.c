#include "harp.h"
#include <esp_log.h>
#include <esp_check.h>
#include "notes.h"


static const char *TAG = "harp";

static const int harp_scale[8][8] __attribute__((unused)) = {
    { -1, C5, Db5,A5, Bb5,Gb6,-1, D7 },
    { C4, Db4,D5, Eb5,B5, -1, G6, Ab6},
    { Gb3,D4, Eb4,E5, -1, C6, Db6,A6 },
    { G3, Ab3,E4, -1, F5, Gb5,D6, Eb6},
    { Db3,A3, -1, F4, Gb4,G5, Ab5,E6 },
    { D3, -1, Bb3,B3, G4, Ab4,A5, Bb5},
    { -1, Eb3,E3, C4, Db4,A4, Bb4,B5 },
    { Ab2,A2, F3, Gb3,D4, Eb4,B4, -1 }
};


static void harp_state_init(harp_state_t *state) {
    state->x = 0;
    state->y = 0;
    state->active = false;
    state->note = 0;
}

esp_err_t harp_init(harp_t *harp, const harp_config_t *config) {
    harp->config = *config;
    harp->flipped = false;

    harp_state_init(&harp->current);
    harp_state_init(&harp->previous);
    harp_state_init(&harp->prevraw);

    const sensor_config_t sensor_config = {
        .num_channels = harp->config.size,
        .index_lut = harp->config.sensor_index_lut,
        .adc = {
            .width = ADC_WIDTH_BIT_12,
            .atten = ADC_ATTEN_DB_11
        },
        .calibration = {
            .iterations = harp->config.calibration_iterations,
            .threshold = 120
        }
    };
    ESP_RETURN_ON_ERROR(sensor_init(&harp->sensor, &sensor_config),
        TAG, "failed to initialize sensor");

    // initiate the calibration process
    ESP_RETURN_ON_ERROR(sensor_calibrate(&harp->sensor),
        TAG, "failed to calibrate sensor");

    return ESP_OK;
}

static int harp_read_sensors(harp_t *harp, int offset, int previous) {
    int i, current, count;

    // if the previously active index is still HIGH, prefer that one and return it
    // that way, if multiple sensors get activated, the first combination will remain active
    // until released
    if (previous != -1 && harp->sensor.state.current & (1 << (offset + previous))) {
        return previous;
    }

    // search for the first active sensor's index
    for (i = 0, count = 0; i < harp->config.size; i++) {
        if (harp->sensor.state.current & (1 << (offset + i))) {
            current = i;
            count++;
        }
    }

    // exactly one sensor must be active
    if (count == 1) return current;
    else return -1;
}

esp_err_t harp_update(harp_t *harp) {
    int8_t note;

    ESP_RETURN_ON_ERROR(sensor_update(&harp->sensor),
        TAG, "failed to update sensor");

    // get the current x and y position
    harp->current.x = harp_read_sensors(harp, 0, harp->previous.x);
    harp->current.y = harp_read_sensors(harp, harp->config.size, harp->previous.y);

    // return if the position did not change
    if (harp->current.x == harp->previous.x && harp->current.y == harp->previous.y) return ESP_OK;

    // get the active note
    harp->current.active = false;

    if (harp->current.x != -1 && harp->current.y != -1) {
        note = harp_scale[harp->config.size - 1 - harp->current.y][harp->config.size - 1 - harp->current.x];

        if (note != -1) {
            harp->current.active = true;
            harp->current.note = note;

            // calculate the velocity
            /* activationTime = millis() - current.timestamp;
            if (activationTime > HARP_MAX_ACTIVATION_TIME) {
                velocity = 1;
            } else {
                velocity = 127 - (activationTime * 126) / HARP_MAX_ACTIVATION_TIME;
            } */
        }
    }

    /*
    // release previous note
    if (harp->previous.active) {
        printf("RELEASE %d %d -> %d\n", harp->previous.x, harp->previous.y, harp->previous.note);
    }

    // press current note
    if (harp->current.active) {
        printf("PRESS %d %d -> %d\n", harp->current.x, harp->current.y, harp->current.note);
    }
    */

    // release previous note
    if (harp->previous.active) {
        putchar(0x90);
        putchar(harp->previous.note);
        putchar(0);
    }

    // press current note
    if (harp->current.active) {
        putchar(0x90);
        putchar(harp->current.note);
        putchar(127);
    }
    fflush(stdout);

    harp->previous = harp->current;
    return ESP_OK;
}
