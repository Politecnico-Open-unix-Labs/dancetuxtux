#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h> // uint8_t, uint16_t
#include <stddef.h> // NULL pointer

#define USE_BINARY_BUFFER // define to enable binary buffer memory optimization (requires more computing power)

#ifdef USE_BINARY_BUFFER
#    define CIRCULAR_BUFFER_BUF_LEN(SAMPLES_NUM) ((SAMPLES_NUM + CHAR_BIT - 1)/CHAR_BIT) // Rounds up
#else // not using binary buffer
#    define CIRCULAR_BUFFER_BUF_LEN(SAMPLES_NUM) (SAMPLES_NUM) // One byte per sample
#endif // USE_BINARY_BUFFER

// Binary buffer (i.e. can store only 1s and 0s)
struct _circular_buffer_t {
    uint8_t * data; // pointer to the beginning of the array
    uint8_t len; // Lenght of the buffer (wich is not lenght of the data)
    uint8_t pos; // current position in the buffer
    uint8_t sum; // Sum of all the data in the buffer
};

typedef struct _circular_buffer_t * circular_buffer_ptr_t;
typedef const struct _circular_buffer_t * circular_buffer_const_ptr_t;
typedef struct _circular_buffer_t circular_buffer_t[1]; // util name

void circular_buffer_init(circular_buffer_ptr_t buf, uint8_t * data, uint8_t len); // inits a buffer
void circular_buffer_reset(circular_buffer_ptr_t buf);
void circular_buffer_push(circular_buffer_ptr_t buf, uint8_t new_data); // pushes new data (popping the old one)
uint8_t circular_buffer_sum(circular_buffer_const_ptr_t buf); // gets the sum of all the data into the buffer

#endif // CIRCULAR_BUFFER_H defined
