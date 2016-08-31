#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h> // uint8_t

// If you don't know what next parameters are leave them as they are
#define SAMPLES_NUM 64 // Number of samples to take before choosing whether the button is pressed or not
#define LOW_THREESHOLD  0.90 // must be in [0.00-1.00], choosing parameter
#define HIGH_THREESHOLD 0.10 // must be in [0.00-1.00], as above

// Set next two to zero to disable the effect
#define PRESS_THREESHOLD 1 // increase in sensor threeshold before keypress
#define RELEASE_THREESHOLD 0 // same as above for keyrelease (sensor threeshold decrease)
#define HYSTERESIS 10 // Time to wait before keyrelease
#define MAX_TIME_IN_GRAYZONE ((uint32_t)1E9) // uinsigned int between 0 and 2^32 -1
                        // After that time in grayzone sensibility is reset
#define MAX_PROBE_STEPS ((uint32_t)250) // sometimes probe gets stuck (don't know why).
                        // This forces exit after given number of steps

#define INPUT_PIN_UP    8
#define INPUT_PIN_DOWN  9
#define INPUT_PIN_LEFT  10
#define INPUT_PIN_RIGHT 11

// Do not change the name of the following variables
// You should not include this file outside main.cpp

// Array of input pins
const uint8_t inputs[] = {INPUT_PIN_UP, INPUT_PIN_DOWN, INPUT_PIN_LEFT, INPUT_PIN_RIGHT};
const uint8_t inputs_len = sizeof(inputs)/sizeof(*inputs); // number of inputs

// TODO: array of calling functions
// TODO: use progmem

#endif // CONFIG_H defined
