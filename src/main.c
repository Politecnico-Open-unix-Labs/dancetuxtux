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
/*
    If you enjoy this work and you would like to support this project,
    please, consider donating me some bitcoins
          ------ 1NFZcpPRPchSfWAHCBqXrKAzXKrYVHL9ca ------
    Any amount, also a single satoshi, is always well accepted!
    You will get fantastic updates!
*/

#include <stdint.h> // uint8_t
#include <limits.h>

#include "cpu.h" // All the needed to work with the microcontroller
#include "config.h" // Necessary configuration file
#include "capacitive.h" // To read capacitive pins
#include "circular_buffer.h" // all circular_buffer features
#include "USB.h" // Usb communication

// Arrow codes
#define KEY_UP_ARROW      82
#define KEY_DOWN_ARROW    81
#define KEY_LEFT_ARROW    80
#define KEY_RIGHT_ARROW   79

int main(void) {
    __USB_init();
    inint_inputs(inputs, inputs_len); // Inits capacitive inputs
    init_capacitive_sensor();

    DDRC |= _BV(7); // output
    _MemoryBarrier(); // Forces executing r/w ops in order

    while (1) {
        if (capacitive_pressed()) {
            __USB_send_keypress(KEY_UP_ARROW); // Requires about 2ms to complete
            PORTC |= _BV(7); // Switchs Arduino Led on
        } else {
            __USB_send_keyrelease(KEY_UP_ARROW); // Requires abotu 2ms to complete
            PORTC &= ~(_BV(7)); // Switches Arduino Led off
        }

        _delay_ms(10); // TODO: use timer interrupts instead
    }
    return 0;
}
