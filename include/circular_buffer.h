#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h> // uint8_t, uint16_t
#include <stddef.h> // NULL pointer

struct _circular_buffer_t {
    uint8_t * data; // pointer to the beginning of the array
    uint8_t len; // Lenght of the data
    uint8_t pos; // current position in the buffer

    uint16_t sum; // Sum of all the data in the buffer
};

typedef struct _circular_buffer_t * circular_buffer_ptr_t;
typedef const struct _circular_buffer_t * circular_buffer_const_ptr_t;
typedef struct _circular_buffer_t circular_buffer_t[1];

void circular_buffer_init(circular_buffer_ptr_t buf, uint8_t * data, uint8_t len); // inits a buffer
void circular_buffer_reset(circular_buffer_ptr_t buf);
void circular_buffer_push(circular_buffer_ptr_t buf, uint8_t new_data); // pushes new data (popping the old one)
uint16_t circular_buffer_sum(circular_buffer_const_ptr_t buf); // gets the sum of all the data into the buffer

#endif // CIRCULAR_BUFFER_H defined
