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
#include "capacitive.h" // To read capacitive pins
#include "circular_buffer.h" // all circular_buffer features
#include "USB.h" // Usb communication

#ifdef USE_ARDUINO_LED
#  define ARDUINO_LED_INIT() DDRC |= _BV(7)
#  define ARDUINO_LED_ON()   PORTC |= _BV(7)
#  define ARDUINO_LED_OFF()  PORTC &= ~(_BV(7))
    static uint8_t switchon_led = 0; // counter
#  define ONE_MORE_SWITCH() switchon_led++
#  define ONE_LESS_SWITCH() switchon_led--
#  define AT_LEAST_ONE_SWITCH() (switchon_led == 0)
#else // Defines the same macros, without any arg
#  define ARDUINO_LED_INIT()
#  define ARDUINO_LED_ON()
#  define ARDUINO_LED_OFF()
#  define ONE_MORE_SWITCH()
#  define ONE_LESS_SWITCH()
#  define AT_LEAST_ONE_SWITCH() 0 // Always false
#endif // USE_ARDUINO_LED

// Arrow codes
#define KEY_UP_ARROW      82
#define KEY_DOWN_ARROW    81
#define KEY_LEFT_ARROW    80
#define KEY_RIGHT_ARROW   79

// Handler functions
#define CREATE_PRESS_HANDLER(name)                                              \
        void handler_ ## name ## _keypress(void) {                              \
            /* Atual send key, Requires about 2ms to complete */                \
            __USB_send_keypress(KEY_ ## name ## _ARROW);                        \
            ARDUINO_LED_ON(); /* Switchs Arduino Led on */                      \
            ONE_MORE_SWITCH();                                                  \
        }                                                                       \
        void handler_ ## name ## _keypress(void)

#define CREATE_RELEASE_HANDLER(name)                                            \
        void handler_ ## name ## _keyrelease(void) {                            \
            /* Atual send key, Requires about 2ms to complete */                \
            __USB_send_keyrelease(KEY_ ## name ## _ARROW);                      \
            ONE_LESS_SWITCH(); /* Switches Arduino Led off */                   \
            if (AT_LEAST_ONE_SWITCH())                                          \
                ARDUINO_LED_OFF(); /* Switches Arduino Led off */               \
        }                                                                       \
        void handler_ ## name ## _keyrelease(void)

// All of the next macros will crerate the needed functions
CREATE_PRESS_HANDLER(UP);
CREATE_PRESS_HANDLER(DOWN);
CREATE_PRESS_HANDLER(LEFT);
CREATE_PRESS_HANDLER(RIGHT);

CREATE_RELEASE_HANDLER(UP);
CREATE_RELEASE_HANDLER(DOWN);
CREATE_RELEASE_HANDLER(LEFT);
CREATE_RELEASE_HANDLER(RIGHT);
#include "config.h" // Necessary configuration file (needs function definition)

#include <alloca.h>
int freeRam () {
    extern int __heap_start, *__brkval;
    uint8_t * v = alloca(sizeof(uint8_t));
    return (int)v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

int main(void) {
    uint8_t stat, i; // status of sensors, counter
    capacitive_sensor_t sensors[inputs_len]; // One for each input
    handler_t temp; // Pointer to the function to execute

    if (freeRam() < 2048) {
        // Makes arduino led blink fast, forever
        ARDUINO_LED_INIT();
        while (1) {
            ARDUINO_LED_ON();
            _delay_ms(100);
            ARDUINO_LED_OFF();
            _delay_ms(100);
        }
    }

    // Does all the initializations
    __USB_init(); // Automatically do all the needed to init usb
#ifdef USE_PROGMEM
    capacitive_sensor_inits_P(sensors, inputs, inputs_len); // Calls the PROGMEM version
#else
    capacitive_sensor_inits(sensors, inputs, inputs_len); // Calls the npn-PROGMEM version
#endif

    ARDUINO_LED_INIT(); // sets Arduino LED as output
    _MemoryBarrier(); // Forces executing r/w ops in order

    while (1) {
        stat = capacitive_sensor_pressed(sensors, inputs_len);
        for (i = 0; i < inputs_len; i++) {
            if (stat & _BV(i)) // pressed the i-th key
                temp = pgm_read_ptr_near(keypress_handlers + i);
            else
                temp = pgm_read_ptr_near(keyrelease_handlers + i);

            if (temp != NULL) // Now executes the function
                temp();
        }

        _delay_ms(10); // TODO: use timer interrupts instead
    }
    return 0;
}
