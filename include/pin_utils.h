#ifndef PIN_UTILS_H
#define PIN_UTILS_H

#include <stdint.h> // uint8_t

typedef struct {
    volatile uint8_t * port, * ddr, * pin; // pointers to port variable
    uint8_t bitmask; // bitmask for that port
} pin_t;

pin_t id_to_pin(uint8_t id);

#endif
