#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h> // uint8_t

#define INPUT_PIN_UP    8
#define INPUT_PIN_DOWN  9
#define INPUT_PIN_LEFT  10
#define INPUT_PIN_RIGHT 11

// Do not change the name of the following variables

// Declares variables as extern const.
// It is necessary, because declaring a variable as const makes automatically static
extern const uint8_t inputs[];
extern const uint8_t inputs_len;

// Array of input pins
const uint8_t inputs[] = {INPUT_PIN_UP, INPUT_PIN_DOWN, INPUT_PIN_LEFT, INPUT_PIN_RIGHT};
const uint8_t inputs_len = sizeof(inputs)/sizeof(*inputs); // number of inputs

// TODO: array of calling functions

#endif // CONFIG_H defined
