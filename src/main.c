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

#include <stdint.h> // uint8_t

#include "cpu.h" // All the needed to work with the microcontroller
#include "config.h" // Necessary configuration file
#include "capacitive.h" // To read capacitive pins
#include "circular_buffer.h" // all circular_buffer features
#include "USB.h" // Usb communication

int main() {
    uint8_t i; // counter
    uint8_t buffers[BUF_LEN];
    circular_buffer_t buf;

    __USB_init();

    circular_buffer_init(buf, buffers, BUF_LEN);
    for (i = 0; i < BUF_LEN; i++) // Fills with 50% 1 and 50% zero
        circular_buffer_push(buf, i % 2);

    DDRC = _BV(7); // output
    _MemoryBarrier(); // Forces executing r/w ops in order

    inint_inputs(inputs, inputs_len);
    set_threshold(12);
    while (1) {
        if (check_port(8)) circular_buffer_push(buf, 1);
        else               circular_buffer_push(buf, 0);

        if (circular_buffer_sum(buf) >= BUF_LEN*0.75)
            PORTC |= _BV(7);
        else
            PORTC &= ~(_BV(7));

        if (circular_buffer_sum(buf) >= BUF_LEN*0.90)
            set_threshold(get_threshold() + 1);

        if (circular_buffer_sum(buf) <= BUF_LEN*0.10)
            set_threshold(get_threshold() - 1);

        discharge_ports();
        _delay_ms(10);
    }
    return 0;
}
