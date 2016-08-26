#include <string.h> // memset
#include "circular_buffer.h"

void circular_buffer_reset(struct _circular_buffer_t * const buf) {
    memset(buf->data, 0, (buf->len)*sizeof*(buf->data)); // fills the buffer with 0s
    buf->pos = 0; // This is arbitrary, but must be less than 'len'
    buf->sum = 0; // sum of all data
}

void circular_buffer_init(struct _circular_buffer_t * const buf, uint8_t * data, const uint8_t len) {
    buf->len = len;
    buf->data = data;
    circular_buffer_reset(buf);
}

void circular_buffer_push(struct _circular_buffer_t * const buf, const uint8_t new_data) {
    buf->sum -= buf->data[buf->pos]; // subtracts old data
    buf->data[buf->pos] = new_data; // writes new in the buffer
    buf->sum += buf->data[buf->pos]; // adds new data
    (buf->pos)++; // Increases the pointer
    if (buf->pos >= buf->len)
        buf->pos = 0; // Resets when pos reaches the end
}

uint16_t circular_buffer_sum(const struct _circular_buffer_t * const buf) {
    return buf->sum;
}
