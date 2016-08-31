/*
    DanceTuxTux Board Project
    Copyright (C) 2016  Serraino Alessio

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h> // memset
#include <limits.h>
#include "circular_buffer.h"
#include "cpu.h" // _BV definition

#ifdef USE_BINARY_BUFFER
#    define BIT_OF(address)  (address % CHAR_BIT)
#    define BYTE_OF(address) (address / CHAR_BIT)
#    define GET_BIT_AT(buffer, address) (!!( buffer[BYTE_OF(address)] & _BV(BIT_OF(address)) ))
#    define SET_BIT_AT(buffer, address) (buffer[BYTE_OF(address)] |= _BV(BIT_OF(address)) )
#    define CLR_BIT_AT(buffer, address) (buffer[BYTE_OF(address)] &= ~(_BV(BIT_OF(address))) )
#    define WRITE_BIT_AT(buffer, address, val) (val)?SET_BIT_AT(buffer, address):CLR_BIT_AT(buffer, address)
#else // USE_BINARY_BUFFER not defined
#    define GET_BIT_AT(buffer, address)        (!!buffer[address]) // Gets binary value
#    define WRITE_BIT_AT(buffer, address, val) (buffer[address] = !!val) // Sets binary value
#endif // USE_BINARY_BUFFER

void circular_buffer_reset(struct _circular_buffer_t * const buf) {
    uint8_t i; // counter

    memset(buf->data, 0, CIRCULAR_BUFFER_BUF_LEN(buf->len)*sizeof*(buf->data)); // fills the buffer with 0s
    buf->pos = 0; // This is arbitrary, but must be less than 'len'
    buf->sum = 0; // sum of all data

    for (i = 0; i < buf->len; i++) // Fills with 50% 1 and 50% zero
        circular_buffer_push(buf, i % 2);
}

void circular_buffer_init(struct _circular_buffer_t * const buf, uint8_t * data, const uint8_t len) {
    buf->len = len;
    buf->data = data;
    circular_buffer_reset(buf);
}

void circular_buffer_push(struct _circular_buffer_t * const buf, const uint8_t new_data) {
    buf->sum -= GET_BIT_AT(buf->data, buf->pos); // subtracts old data
    WRITE_BIT_AT(buf->data, buf->pos, new_data); // writes new in the buffer
    buf->sum += GET_BIT_AT(buf->data, buf->pos); // adds new data
    (buf->pos)++; // Increases the pointer
    if (buf->pos >= buf->len)
        buf->pos = 0; // Resets when pos reaches the end
}

uint8_t circular_buffer_sum(const struct _circular_buffer_t * const buf) {
    return buf->sum;
}
