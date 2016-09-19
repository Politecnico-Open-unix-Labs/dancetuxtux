#ifndef CAPACITIVE_H
#define CAPACITIVE_H

#include <stdint.h> // uint8_t
#include "circular_buffer.h" // circular_buffer_t
#include "capacitive_settings.h"

// Lenght of a buffer containing SAMPLES_NUM samples
#define BUF_LEN CIRCULAR_BUFFER_BUF_LEN(SAMPLES_NUM)
__attribute__((packed))
struct capacitive_sensor_t {
    uint8_t low_buffer_data[BUF_LEN];  // Actual buffer where data is stored
    uint8_t high_buffer_data[BUF_LEN]; // Actual buffer where data is stored
    circular_buffer_t low_buffer, high_buffer; // Buffer wrapper
    uint8_t high_threshold, low_threshold; // between 0 and 255
    uint16_t hysteresis_a, hysteresis_b; // Avoid send a double press when actually is only one
    uint32_t gray_zone; // Time in spent gray zone
    uint8_t pin:6; // pin on which to execute the measurement
    uint8_t to_probe:1; // true if needed to re-calibrate the sensor
    uint8_t pressed:1; // true wether pressed, else false
};

typedef struct capacitive_sensor_t   capacitive_sensor_t;
typedef struct capacitive_sensor_t * capacitive_sensor_ptr_t;

void capacitive_sensor_init(const capacitive_sensor_ptr_t sensors, const uint8_t pin_id);
void capacitive_sensor_inits(const capacitive_sensor_ptr_t sensors, const uint8_t * const pin_id, const uint8_t num);
void capacitive_sensor_inits_P(const capacitive_sensor_ptr_t sensors, const uint8_t * const pin_id, const uint8_t num);
uint32_t capacitive_sensor_pressed(const capacitive_sensor_ptr_t sensors, const uint8_t num);

// -----------------------------------
// -----   LOW LEVEL FUNCTIONS   -----
// -----------------------------------

/* Inits pin to use as capacitive sensor */
void inint_inputs(const uint8_t inputs[], const uint8_t inputs_len);
void inint_inputs_P(const uint8_t inputs[], const uint8_t inputs_len);

/* Should call after each round of capacitive reads (TODO: automatic process?) */
void discharge_ports(void);

/* Capacitive read function,
   returns 1 if there is contact, 0 if not, and -1 on errors */
unsigned char check_port(uint8_t in);
void set_threshold(uint8_t new_sens);
uint8_t get_threshold(void);

#endif // CAPACITIVE_H defined
